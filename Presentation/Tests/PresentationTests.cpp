#include "ARUI/Language/node.hpp"
#include "ARUI/Presentation/PresentationRpcServer.hpp"
#include "ARUI/Presentation/RpcPresentationController.hpp"
#include "ARUI/Presentation/RuntimePresentationController.hpp"
#include "ARUI/Runtime/RuntimeTree.hpp"

#include <array>
#include <gtest/gtest.h>
#include <mutex>

using namespace ARUI;

TEST(PresentationRpc, AppliesPrivilegedOperationsToRuntimeTree) {
  Runtime::RuntimeTree runtime;
  std::mutex runtimeMutex;
  Presentation::RuntimePresentationController service(runtime, runtimeMutex);
  constexpr std::uint16_t port = 44243;
  Presentation::PresentationRpcServer endpoint(service, port);
  endpoint.Start();
  Presentation::RpcPresentationController client("127.0.0.1", port);

  Language::Style surfaceStyle;
  surfaceStyle.width = {1.0, Language::LengthUnit::Meter};
  surfaceStyle.height = {75.0, Language::LengthUnit::Centimeter};
  surfaceStyle.zOffset = {-2.0, Language::LengthUnit::Meter};
  surfaceStyle.painterProperties["label"] = std::string{"desktop"};
  const auto surface = client.CreateTree(
      Presentation::PresentationRoot,
      Language::LSurface({Language::LGroup()}, {}, surfaceStyle));

  const auto *surfaceNode = runtime.Get(surface);
  ASSERT_NE(surfaceNode, nullptr);
  ASSERT_EQ(surfaceNode->children.size(), 1u);
  EXPECT_EQ(surfaceNode->style.width, surfaceStyle.width);
  EXPECT_EQ(
      std::get<std::string>(surfaceNode->style.painterProperties.at("label")),
      "desktop");
  const auto group = surfaceNode->children.front();

  const auto first = client.CreateNode(group, Language::LText("first"));
  const auto second = client.CreateNode(group, Language::LText("second"));
  const std::array reordered{second, first};
  client.ReorderChildren(group, reordered);
  EXPECT_EQ(runtime.Get(group)->children,
            (std::vector<Runtime::NodeID>{second, first}));

  client.SetVisibility(first, false);
  EXPECT_FALSE(runtime.Get(first)->visible);
  Language::Style textStyle;
  textStyle.fontSize = {8.0, Language::LengthUnit::Millimeter};
  client.SetStyle(first, textStyle,
                  {.duration = std::chrono::milliseconds{250},
                   .easing = Presentation::Easing::EaseInOut});
  EXPECT_EQ(runtime.Get(first)->style.fontSize, textStyle.fontSize);
  EXPECT_EQ(runtime.LastCommitOptions().duration,
            std::chrono::milliseconds{250});
  EXPECT_EQ(runtime.LastCommitOptions().easing, Runtime::Easing::EaseInOut);

  client.Move(first, surface);
  EXPECT_EQ(runtime.Get(first)->parent, surface);
  client.Remove(second);
  EXPECT_EQ(runtime.Get(second), nullptr);
  client.Reset();
  EXPECT_TRUE(runtime.RootChildren().empty());
}
