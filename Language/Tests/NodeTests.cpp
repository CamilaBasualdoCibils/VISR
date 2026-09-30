#include "ARUI/Language/node.hpp"
#include <gtest/gtest.h>

using namespace ARUI::Language;

TEST(NodeStyle, PresentationIsStoredSeparatelyFromAttributes) {
  Style style;
  style.width = Length{120, LengthUnit::Pixel};
  auto button =
      LButton(ActionReference{"submit"}, {}, {.disabled = true}, style);
  EXPECT_FALSE(button.HasAttribute("width"));
  EXPECT_TRUE(*button.GetAttribute<bool>("disabled"));
  EXPECT_EQ(button.GetAttribute<ActionReference>("action")->value, "submit");
  ASSERT_FALSE(button.style.width.IsAuto());
  EXPECT_EQ(button.style.width.value, 120);
  EXPECT_EQ(button.style.width.unit, LengthUnit::Pixel);
}

TEST(NodeStyle, SurfacePresentationAndSemanticsAreSeparated) {
  Style style;
  style.shape = "cylinder";
  style.radius = Length{80, LengthUnit::Centimeter};
  style.arc = Angle{60, AngleUnit::Degree};
  auto surface = LSurface(
      {}, {.anchor = "left-forearm", .surfaceType = SurfaceType::Cylinder},
      style);
  EXPECT_EQ(*surface.GetAttribute<std::string>("anchor"), "left-forearm");
  EXPECT_EQ(surface.surfaceType, SurfaceType::Cylinder);
  EXPECT_FALSE(surface.HasAttribute("shape"));
  EXPECT_EQ(surface.style.shape, "cylinder");
  EXPECT_EQ(surface.style.radius->value, 80);
  EXPECT_EQ(surface.style.arc->value, 60);
}

TEST(NodeStyle, SemanticHelperOptionsRemainAttributes) {
  auto panel = LPanel({}, {.name = "Map", .script = "Maps/Map.js"});
  EXPECT_EQ(*panel.GetAttribute<std::string>("name"), "Map");
  EXPECT_EQ(*panel.GetAttribute<std::string>("script"), "Maps/Map.js");
  auto text = LText(StateReference{"Music/Title"});
  EXPECT_EQ(text.GetAttribute<StateReference>("state")->value, "Music/Title");
}

TEST(PainterStyle, StoresExtensiblePropertiesAndValidatesSchemaTypes) {
  PainterStyleSchema schema{{
      {"character", PainterStyleType::String, true},
      {"spacing", PainterStyleType::Length, true},
  }};
  Style style;
  style.painter = PainterReference{"ascii"};
  style.painterProperties["character"] = std::string{"#"};
  style.painterProperties["spacing"] = Length{4, LengthUnit::Millimeter};
  EXPECT_TRUE(
      schema.Accepts("character", style.painterProperties.at("character")));
  EXPECT_TRUE(schema.Accepts("spacing", style.painterProperties.at("spacing")));
  EXPECT_FALSE(schema.Accepts("spacing", PainterStyleValue{2.0}));
  EXPECT_FALSE(schema.Accepts("unknown", PainterStyleValue{std::string{"x"}}));
  ASSERT_NE(schema.Find("character"), nullptr);
  EXPECT_TRUE(schema.Find("character")->inherited);
}

TEST(NodeStyle, DimensionsDefaultToAutoAndCanHoldExplicitLengths) {
  Style style;
  EXPECT_TRUE(style.width.IsAuto());
  EXPECT_TRUE(style.height.IsAuto());

  style.width = Length{75, LengthUnit::Percent};
  ASSERT_FALSE(style.width.IsAuto());
  EXPECT_EQ(style.width.value, 75);
  EXPECT_EQ(style.width.unit, LengthUnit::Percent);
}
