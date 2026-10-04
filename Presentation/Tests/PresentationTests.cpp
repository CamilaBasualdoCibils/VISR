#include "ARUI/Language/node.hpp"
#include "ARUI/Presentation/PresentationRpcServer.hpp"
#include "ARUI/Presentation/RpcPresentationController.hpp"
#include "ARUI/Presentation/RuntimePresentationController.hpp"
#include "ARUI/Runtime/RuntimeTree.hpp"
#include <gtest/gtest.h>
#include <mutex>
using namespace ARUI;
TEST(PresentationRpc, ControlsActiveLanguageTree) {
  Runtime::RuntimeTree runtime; std::mutex mutex;
  Presentation::RuntimePresentationController service(runtime, mutex);
  Presentation::PresentationRpcServer endpoint(service, 44243); endpoint.Start();
  Presentation::RpcPresentationController client("127.0.0.1", 44243);
  client.SetActiveTree(Language::LSurface({Language::LPanel()}));
  auto tree = client.GetActiveTree(); ASSERT_EQ(tree.type, Language::LNodeType::Surface);
  ASSERT_EQ(tree.children.size(), 1u); const auto panel = tree.children.front().id;
  client.AddTree(panel, Language::LText("first")); tree = client.GetActiveTree();
  const auto text = tree.children.front().children.front().id;
  Language::PanelOptions options; options.script = "script here";
  client.UpdateNode(panel, Language::LPanel({}, options));
  EXPECT_EQ(std::get<std::string>(client.GetActiveTree().children.front().attributes.at("script")), "script here");
  client.Remove(panel); EXPECT_TRUE(client.GetActiveTree().children.empty());
  client.Clear(); EXPECT_TRUE(runtime.RootChildren().empty());
}
