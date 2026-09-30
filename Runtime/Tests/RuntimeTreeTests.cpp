#include "ARUI/Application/Application.hpp"
#include "ARUI/Runtime/RuntimeTree.hpp"

#include <gtest/gtest.h>

using namespace ARUI;

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
TEST(RuntimeTree, InsertsAndLooksUpSingleNode) {
  Runtime::RuntimeTree runtime;
  auto tx = runtime.BeginTransaction();
  const auto id = tx.Insert(runtime.Root(), Language::LSurface());
  tx.Commit();
  ASSERT_NE(runtime.Get(id), nullptr);
  EXPECT_EQ(runtime.Get(id)->id, id);
  EXPECT_EQ(runtime.Get(id)->parent, runtime.Root());
  EXPECT_EQ(runtime.RootChildren(), std::vector{id});
}

TEST(RuntimeTree, AcceptsMultipleSurfaceRootNodes) {
  Runtime::RuntimeTree runtime;
  auto transaction = runtime.BeginTransaction();
  const auto first = transaction.Insert(runtime.Root(), Language::LSurface());
  const auto second = transaction.Insert(runtime.Root(), Language::LSurface());
  transaction.Commit();

  EXPECT_EQ(runtime.RootChildren(), (std::vector{first, second}));
}

TEST(RuntimeTree, RecursivelyInstantiatesLanguageTree) {
  const auto languageTree = Language::LSurface(
      {Language::LRow({Language::LText("first"), Language::LText("second")})});
  Runtime::RuntimeTree runtime;
  auto tx = runtime.BeginTransaction();
  const auto surface = tx.InsertTree(runtime.Root(), languageTree);
  tx.Commit();

  const auto row = runtime.Get(surface)->children.at(0);
  ASSERT_EQ(runtime.Get(row)->children.size(), 2);
  EXPECT_EQ(runtime.Get(row)->parent, surface);
  EXPECT_EQ(runtime.Get(runtime.Get(row)->children[0])->parent, row);
  EXPECT_EQ(
      std::get<std::string>(
          runtime.Get(runtime.Get(row)->children[0])->attributes.at("text")),
      "first");
}

TEST(RuntimeTree, InstantiatesApplicationTemplateThroughSamePath) {
  Application::TemplateDescriptor descriptor{
      .id = 7,
      .name = "player",
      .description = "Player panel",
      .root =
          Language::LSurface({Language::LPanel({Language::LText("track")})})};
  Runtime::RuntimeTree runtime;
  auto tx = runtime.BeginTransaction();
  const auto panel = tx.InsertTree(runtime.Root(), descriptor.root);
  tx.Commit();

  ASSERT_NE(runtime.Get(panel), nullptr);
  EXPECT_EQ(runtime.Get(panel)->type, Language::LNodeType::Surface);
  EXPECT_EQ(runtime.Get(panel)->children.size(), 1);
}

TEST(RuntimeTree, RemovesNodeAndItsDescendants) {
  Runtime::RuntimeTree runtime;
  auto insert = runtime.BeginTransaction();
  const auto parent = insert.InsertTree(
      runtime.Root(), Language::LSurface({Language::LText("child")}));
  const auto child = parent + 1;
  insert.Commit();

  auto remove = runtime.BeginTransaction();
  remove.Remove(parent);
  remove.Commit();
  EXPECT_EQ(runtime.Get(parent), nullptr);
  EXPECT_EQ(runtime.Get(child), nullptr);
  EXPECT_TRUE(runtime.RootChildren().empty());
}

TEST(RuntimeTree, ReparentsWithoutChangingIdentity) {
  Runtime::RuntimeTree runtime;
  auto insert = runtime.BeginTransaction();
  const auto left = insert.Insert(runtime.Root(), Language::LSurface());
  const auto right = insert.Insert(runtime.Root(), Language::LSurface());
  const auto panel = insert.Insert(left, Language::LPanel());
  insert.Commit();

  auto move = runtime.BeginTransaction();
  move.Move(panel, right);
  move.Commit();
  EXPECT_EQ(runtime.Get(panel)->id, panel);
  EXPECT_EQ(runtime.Get(panel)->parent, right);
  EXPECT_TRUE(runtime.Get(left)->children.empty());
  EXPECT_EQ(runtime.Get(right)->children, std::vector{panel});
}

TEST(RuntimeTree, AppliesSeveralChangesAtomically) {
  Runtime::RuntimeTree runtime;
  auto insert = runtime.BeginTransaction();
  const auto surface = insert.Insert(runtime.Root(), Language::LSurface());
  const auto panel = insert.Insert(surface, Language::LPanel());
  const auto first = insert.Insert(panel, Language::LText("first"));
  const auto second = insert.Insert(panel, Language::LText("second"));
  insert.Commit();

  Language::Style style;
  style.width = Language::Length{50, Language::LengthUnit::Centimeter};
  auto update = runtime.BeginTransaction();
  update.SetStyle(panel, style);
  update.SetVisibility(panel, false);
  update.ReorderChildren(panel, std::vector{second, first});

  EXPECT_TRUE(runtime.Get(panel)->visible);
  EXPECT_TRUE(runtime.Get(panel)->style.width.IsAuto());
  update.Commit({.duration = std::chrono::milliseconds{500},
                 .easing = Runtime::Easing::EaseInOut});

  EXPECT_EQ(runtime.Get(panel)->id, panel);
  EXPECT_FALSE(runtime.Get(panel)->visible);
  EXPECT_FALSE(runtime.Get(panel)->style.width.IsAuto());
  EXPECT_EQ(runtime.Get(panel)->style.width.value, 50);
  EXPECT_EQ(runtime.Get(panel)->style.width.unit,
            Language::LengthUnit::Centimeter);
  EXPECT_EQ(runtime.Get(panel)->children, (std::vector{second, first}));
  EXPECT_EQ(runtime.LastCommitOptions().duration,
            std::chrono::milliseconds{500});
}

TEST(RuntimeTree, RejectsCyclesAndStaleTransactions) {
  Runtime::RuntimeTree runtime;
  auto insert = runtime.BeginTransaction();
  const auto parent = insert.Insert(runtime.Root(), Language::LSurface());
  const auto child = insert.Insert(parent, Language::LGroup());
  insert.Commit();

  auto stale = runtime.BeginTransaction();
  auto current = runtime.BeginTransaction();
  EXPECT_THROW(current.Move(parent, child), std::invalid_argument);
  current.SetVisibility(child, false);
  current.Commit();
  EXPECT_THROW(stale.Commit(), std::logic_error);
}

TEST(RuntimeTree, RejectsNonSurfaceRootNodes) {
  Runtime::RuntimeTree runtime;
  auto transaction = runtime.BeginTransaction();
  EXPECT_THROW(transaction.Insert(runtime.Root(), Language::LPanel()),
               std::invalid_argument);
}
