#include "ARUI/Runtime/Painter/Commons/Layout.hpp"
#include "ARUI/Runtime/Painters/AruiFlatPainter.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <gtest/gtest.h>

using namespace ARUI;

namespace {

constexpr float Epsilon = 1.0e-5F;

Runtime::RuntimeTree MakeTree(Language::LNode root) {
  Runtime::RuntimeTree tree;
  auto transaction = tree.BeginTransaction();
  transaction.InsertTree(tree.Root(), root);
  transaction.Commit();
  return tree;
}

Runtime::PainterCommons::LayoutDefaults
Defaults(Language::Length padding = {0.0, Language::LengthUnit::Meter},
         Language::Length gap = {0.0, Language::LengthUnit::Meter}) {
  return {.padding = padding,
          .gap = gap,
          .fontSize = {6.0, Language::LengthUnit::Millimeter},
          .fontFamily = "Test Sans"};
}

const Runtime::PainterCommons::LayoutNode &
OnlyChild(const Runtime::PainterCommons::LayoutNode &node) {
  EXPECT_EQ(node.children.size(), 1U);
  return node.children.front();
}

class RecordingSink final : public Runtime::DrawingObjectSink {
public:
  void FillPath(const Runtime::FillPathRenderObject &value) override {
    fills.push_back(value);
  }
  void StrokePath(const Runtime::StrokePathRenderObject &value) override {
    strokes.push_back(value);
  }
  void Submit(const Runtime::SurfaceRenderObject &) override {}
  void Submit(const Runtime::ShapeRenderObject &value) override {
    shapes.push_back(value);
  }
  void Submit(const Runtime::CurveRenderObject &) override {}
  void Submit(const Runtime::TextRenderObject &value) override {
    texts.push_back(value);
  }
  void Submit(const Runtime::MeshRenderObject &) override {}

  std::vector<Runtime::FillPathRenderObject> fills;
  std::vector<Runtime::StrokePathRenderObject> strokes;
  std::vector<Runtime::ShapeRenderObject> shapes;
  std::vector<Runtime::TextRenderObject> texts;
};

} // namespace

TEST(PainterCommonsLayout, RowUsesEqualPhysicalWidthsPaddingAndGap) {
  Language::Style rowStyle;
  rowStyle.padding = Language::Length{10, Language::LengthUnit::Centimeter};
  rowStyle.gap = Language::Length{5, Language::LengthUnit::Centimeter};
  auto tree = MakeTree(Language::LSurface({Language::LRow(
      {Language::LPanel(), Language::LPanel()}, {}, rowStyle)}));

  const auto layout = Runtime::PainterCommons::LayoutTree(
      tree, tree.RootChildren().front(), {{0.0F, 0.0F}, {2.0F, 1.0F}},
      Defaults());
  const auto &row = OnlyChild(layout);
  ASSERT_EQ(row.children.size(), 2U);
  EXPECT_NEAR(row.contentBounds.minimum.x, 0.1F, Epsilon);
  EXPECT_NEAR(row.contentBounds.maximum.x, 1.9F, Epsilon);
  EXPECT_NEAR(row.children[0].bounds.Size().x, 0.875F, Epsilon);
  EXPECT_NEAR(row.children[1].bounds.minimum.x, 1.025F, Epsilon);
}

TEST(PainterCommonsLayout, ColumnUsesEqualHeightsAndDefaultSpacing) {
  auto tree = MakeTree(Language::LSurface({Language::LColumn(
      {Language::LPanel(), Language::LPanel(), Language::LPanel()})}));
  const auto layout = Runtime::PainterCommons::LayoutTree(
      tree, tree.RootChildren().front(), {{0.0F, 0.0F}, {1.0F, 1.0F}},
      Defaults({0.1, Language::LengthUnit::Meter},
               {0.05, Language::LengthUnit::Meter}));
  const auto &column = OnlyChild(layout);
  ASSERT_EQ(column.children.size(), 3U);
  EXPECT_NEAR(column.contentBounds.minimum.y, 0.2F, Epsilon);
  EXPECT_NEAR(column.children[0].bounds.Size().y, 0.166667F, Epsilon);
  EXPECT_NEAR(column.children[0].bounds.maximum.y, 0.8F, Epsilon);
  EXPECT_NEAR(column.children[2].bounds.minimum.y, 0.2F, Epsilon);
}

TEST(PainterCommonsLayout, StackOverlapsAndNestedFlowRemainsIndependent) {
  auto tree = MakeTree(Language::LSurface({Language::LStack(
      {Language::LRow({Language::LPanel(), Language::LPanel()}),
       Language::LColumn({Language::LPanel(), Language::LPanel()})})}));
  const auto layout = Runtime::PainterCommons::LayoutTree(
      tree, tree.RootChildren().front(), {{-1.0F, -0.5F}, {1.0F, 0.5F}},
      Defaults());
  const auto &stack = OnlyChild(layout);
  ASSERT_EQ(stack.children.size(), 2U);
  EXPECT_EQ(stack.children[0].bounds.minimum, stack.children[1].bounds.minimum);
  EXPECT_EQ(stack.children[0].bounds.maximum, stack.children[1].bounds.maximum);
  EXPECT_NEAR(stack.children[0].children[0].bounds.Size().x, 1.0F, Epsilon);
  EXPECT_NEAR(stack.children[1].children[0].bounds.Size().y, 0.5F, Epsilon);
}

TEST(PainterCommonsLayout, ExplicitValuesOverrideDefaultsAndFontSizeInherits) {
  Language::Style surfaceStyle;
  surfaceStyle.padding = Language::Length{2, Language::LengthUnit::Centimeter};
  surfaceStyle.gap = Language::Length{1, Language::LengthUnit::Centimeter};
  surfaceStyle.fontSize = Language::Length{8, Language::LengthUnit::Millimeter};
  auto tree = MakeTree(Language::LSurface(
      {Language::LText("first"), Language::LText("second")}, {}, surfaceStyle));
  const auto layout = Runtime::PainterCommons::LayoutTree(
      tree, tree.RootChildren().front(), {{0.0F, 0.0F}, {1.0F, 1.0F}},
      Defaults({0.2, Language::LengthUnit::Meter},
               {0.2, Language::LengthUnit::Meter}));
  EXPECT_NEAR(layout.contentBounds.minimum.x, 0.02F, Epsilon);
  EXPECT_NEAR(layout.children[0].bounds.Size().y, 0.475F, Epsilon);
  EXPECT_EQ(layout.children[0].fontSize,
            (Language::Length{8, Language::LengthUnit::Millimeter}));
  EXPECT_EQ(layout.children[0].fontFamily, "Test Sans");
}

TEST(PainterCommonsLayout, HandlesEmptyInvisibleAndNonPositiveAreasSafely) {
  auto tree = MakeTree(Language::LSurface({Language::LStack({})}));
  auto transaction = tree.BeginTransaction();
  transaction.SetVisibility(tree.Get(tree.RootChildren().front())->children[0],
                            false);
  transaction.Commit();
  const auto layout = Runtime::PainterCommons::LayoutTree(
      tree, tree.RootChildren().front(), {{2.0F, 3.0F}, {1.0F, 1.0F}},
      Defaults({10.0, Language::LengthUnit::Meter},
               {10.0, Language::LengthUnit::Meter}));
  EXPECT_GE(layout.bounds.Size().x, 0.0F);
  EXPECT_GE(layout.bounds.Size().y, 0.0F);
  ASSERT_EQ(layout.children.size(), 1U);
  EXPECT_TRUE(layout.children[0].children.empty());
}

TEST(AruiFlatPainter, EmitsWorldSpaceShapesAndPhysicalText) {
  Language::Style panelStyle;
  panelStyle.painterProperties.emplace(
      "fill", Language::Color{{0.2F, 0.4F, 0.6F, 1.0F}});
  Language::Style textStyle;
  textStyle.fontSize = Language::Length{7, Language::LengthUnit::Millimeter};
  textStyle.painterProperties.emplace("font-family",
                                      std::string{"Physical Sans"});
  auto tree = MakeTree(Language::LSurface(
      {Language::LStack({Language::LPanel({}, {}, panelStyle),
                         Language::LText("hello", {}, textStyle)})}));

  RecordingSink sink;
  Runtime::PaintContext draw{sink};
  Runtime::AruiFlatPainter painter;
  const glm::mat4 localToWorld =
      glm::translate(glm::mat4{1.0F}, {1.0F, 2.0F, 3.0F});
  painter.Paint({.draw = draw,
                 .tree = tree,
                 .root = tree.RootChildren().front(),
                 .localToWorld = localToWorld,
                 .extent = {2.0F, 1.0F}},
                {});

  ASSERT_EQ(sink.shapes.size(), 1U);
  ASSERT_EQ(sink.texts.size(), 1U);
  EXPECT_NEAR(sink.shapes[0].transform.localToWorld[3].x, 1.0F, Epsilon);
  EXPECT_NEAR(sink.shapes[0].transform.localToWorld[3].y, 2.0F, Epsilon);
  EXPECT_GT(sink.shapes[0].transform.localToWorld[3].z, 3.0F);
  EXPECT_EQ(sink.texts[0].fontSize,
            (Language::Length{7, Language::LengthUnit::Millimeter}));
  EXPECT_EQ(sink.texts[0].fontFamily, "Physical Sans");
}

TEST(AruiFlatPainter, InvisibleSubtreesEmitNoPrimitivesAndSchemaIsAccurate) {
  Language::Style panelStyle;
  panelStyle.painterProperties.emplace(
      "fill", Language::Color{{1.0F, 1.0F, 1.0F, 1.0F}});
  auto tree = MakeTree(Language::LSurface(
      {Language::LPanel({Language::LText("hidden")}, {}, panelStyle)}));
  auto transaction = tree.BeginTransaction();
  transaction.SetVisibility(tree.Get(tree.RootChildren().front())->children[0],
                            false);
  transaction.Commit();

  RecordingSink sink;
  Runtime::PaintContext draw{sink};
  Runtime::AruiFlatPainter painter;
  painter.Paint({.draw = draw,
                 .tree = tree,
                 .root = tree.RootChildren().front(),
                 .localToWorld = glm::mat4{1.0F},
                 .extent = {1.0F, 1.0F}},
                {});
  EXPECT_TRUE(sink.shapes.empty());
  EXPECT_TRUE(sink.texts.empty());
  EXPECT_NE(painter.StyleSchema().Find("fill"), nullptr);
  EXPECT_NE(painter.StyleSchema().Find("stroke-width"), nullptr);
  EXPECT_EQ(painter.StyleSchema().Find("font-size"), nullptr);
}
