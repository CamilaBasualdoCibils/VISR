#include "ARUI/Language/Parser.hpp"
#include "ARUI/Language/Serializer.hpp"
#include <gtest/gtest.h>
#include <string_view>
using namespace ARUI::Language;

namespace {
const LNode &OnlySurface(const ParseResult<Document> &parsed) {
  return parsed.value.surfaces.at(0);
}
} // namespace

TEST(Parser, ParsesMinimalDocumentAndPreservesText) {
  const auto parsed = ParseARUI(R"(<arui>
    <surface width="20cm" height="10cm"><text>Hello, ARUI!</text></surface>
  </arui>)");
  ASSERT_TRUE(parsed) << parsed.diagnostics.front().message;
  ASSERT_EQ(parsed.value.surfaces.size(), 1u);
  const auto &surface = OnlySurface(parsed);
  EXPECT_EQ(surface.style.width, (Length{20, LengthUnit::Centimeter}));
  EXPECT_EQ(surface.style.height, (Length{10, LengthUnit::Centimeter}));
  ASSERT_EQ(surface.children.size(), 1u);
  EXPECT_EQ(surface.children[0].type, LNodeType::Text);
  EXPECT_EQ(*surface.children[0].GetAttribute<std::string>("text"),
            "Hello, ARUI!");
  EXPECT_TRUE(surface.children[0].children.empty());
}

TEST(Parser, PreservesNestedHierarchyAndIgnoresCommentsAndWhitespace) {
  const auto parsed = ParseMarkup(R"(<arui><!-- document -->
    <surface width="1m" height="250mm"><column><text>Hello</text>
      <row><panel><text>A</text></panel><panel><text>B</text></panel></row>
    </column></surface></arui>)");
  ASSERT_TRUE(parsed);
  const auto &column = OnlySurface(parsed).children.at(0);
  EXPECT_EQ(column.type, LNodeType::Column);
  ASSERT_EQ(column.children.size(), 2u);
  EXPECT_EQ(column.children[1].type, LNodeType::Row);
  ASSERT_EQ(column.children[1].children.size(), 2u);
  EXPECT_EQ(column.children[1].children[0].type, LNodeType::Panel);
}

TEST(Parser, ParsesMultipleSurfacesAndAllPhysicalUnits) {
  const auto parsed = ParseMarkup(R"(<arui>
    <surface width="20mm" height="10cm"/>
    <surface width="2m" height="30mm"/>
  </arui>)");
  ASSERT_TRUE(parsed);
  ASSERT_EQ(parsed.value.surfaces.size(), 2u);
  EXPECT_EQ(parsed.value.surfaces[0].style.width.unit, LengthUnit::Millimeter);
  EXPECT_EQ(parsed.value.surfaces[0].style.height.unit, LengthUnit::Centimeter);
  EXPECT_EQ(parsed.value.surfaces[1].style.width.unit, LengthUnit::Meter);
}

TEST(Parser, ExtractsStylesScriptsAndBehavior) {
  const auto parsed = ParseMarkup(R"(<arui>
    <style>.control { padding: 5mm; }</style>
    <script type="module">export function play(node) {}</script>
    <surface width="40cm" height="25cm"><panel class="control" behavior="play"/></surface>
  </arui>)");
  ASSERT_TRUE(parsed);
  ASSERT_EQ(parsed.value.stylesheets.size(), 1u);
  EXPECT_NE(parsed.value.stylesheets[0].find("padding: 5mm"),
            std::string::npos);
  ASSERT_EQ(parsed.value.scripts.size(), 1u);
  EXPECT_EQ(parsed.value.scripts[0].type, "module");
  EXPECT_NE(parsed.value.scripts[0].source.find("function play"),
            std::string::npos);
  const auto &panel = OnlySurface(parsed).children.at(0);
  EXPECT_EQ(*panel.GetAttribute<std::string>("behavior"), "play");
}

TEST(Parser, ParsesStateAsTypedReferenceAndImageNode) {
  const auto parsed = ParseMarkup(R"(<arui><surface width="1m" height="1m">
    <text state="system.cpu"/><image src="meter.png" alt="meter"/>
  </surface></arui>)");
  ASSERT_TRUE(parsed);
  const auto &children = OnlySurface(parsed).children;
  EXPECT_EQ(children[0].GetAttribute<StateReference>("state")->value,
            "system.cpu");
  EXPECT_EQ(children[1].type, LNodeType::Image);
}

class InvalidLength : public testing::TestWithParam<const char *> {};
TEST_P(InvalidLength, RejectsUnsupportedUnit) {
  const auto parsed = ParseMarkup(std::string("<arui><surface width=\"1") +
                                  GetParam() + "\" height=\"1m\"/></arui>");
  EXPECT_FALSE(parsed);
  ASSERT_FALSE(parsed.diagnostics.empty());
  EXPECT_NE(parsed.diagnostics[0].message.find("unsupported"),
            std::string::npos);
}
INSTANTIATE_TEST_SUITE_P(UnsupportedUnits, InvalidLength,
                         testing::Values("px", "em", "rem"));

TEST(Parser, RejectsMissingSurfaceWidthAndHeight) {
  auto missingWidth = ParseMarkup("<arui><surface height=\"1m\"/></arui>");
  EXPECT_FALSE(missingWidth);
  EXPECT_NE(missingWidth.diagnostics[0].message.find("width"),
            std::string::npos);
  auto missingHeight = ParseMarkup("<arui><surface width=\"1m\"/></arui>");
  EXPECT_FALSE(missingHeight);
  EXPECT_NE(missingHeight.diagnostics[0].message.find("height"),
            std::string::npos);
}

TEST(Parser, AcceptsDeclaredAttributesAndAssignsTypedValues) {
  const auto parsed = ParseMarkup(R"(<arui>
    <script type="module">export {};</script>
    <surface width="1m" height="25cm" anchor="desk" class="root" behavior="open">
      <group name="tools" class="layout" behavior="groupBehavior">
        <row class="horizontal" behavior="rowBehavior"/>
        <column class="vertical" behavior="columnBehavior"/>
        <stack class="layers" behavior="stackBehavior"/>
        <panel name="details" class="card" behavior="panelBehavior"/>
        <text state="system.cpu" class="value" behavior="textBehavior"/>
        <image src="meter.png" alt="meter" class="icon" behavior="imageBehavior"/>
      </group>
    </surface>
  </arui>)");
  ASSERT_TRUE(parsed) << parsed.diagnostics.front().message;
  const auto &surface = OnlySurface(parsed);
  EXPECT_EQ(*surface.GetAttribute<std::string>("anchor"), "desk");
  EXPECT_EQ(surface.style.width, (Length{1, LengthUnit::Meter}));
  EXPECT_EQ(surface.style.height, (Length{25, LengthUnit::Centimeter}));
  const auto &group = surface.children.at(0);
  EXPECT_EQ(*group.GetAttribute<std::string>("name"), "tools");
  ASSERT_EQ(group.children.size(), 6u);
  EXPECT_EQ(group.children[4].GetAttribute<StateReference>("state")->value,
            "system.cpu");
  EXPECT_EQ(*group.children[5].GetAttribute<std::string>("src"), "meter.png");
  EXPECT_EQ(parsed.value.scripts.at(0).type, "module");
}

TEST(Parser, RejectsUnknownAttributesFromElementSchemas) {
  for (const auto markup : {
           "<arui><surface width='1m' height='1m' bogus='x'/></arui>",
           "<arui><surface width='1m' height='1m'><text "
           "name='x'/></surface></arui>",
           "<arui><script language='js'/></arui>",
       }) {
    const auto parsed = ParseMarkup(markup);
    EXPECT_FALSE(parsed);
    ASSERT_FALSE(parsed.diagnostics.empty());
    EXPECT_NE(parsed.diagnostics[0].message.find("invalid attribute"),
              std::string::npos);
  }
}

TEST(Parser, RejectsMalformedPhysicalLengths) {
  for (const auto value : {"wide", "0cm", "-1m", "1%"}) {
    const auto parsed =
        ParseMarkup("<arui><surface width='" + std::string(value) +
                    "' height='1m'/></arui>");
    EXPECT_FALSE(parsed);
    ASSERT_FALSE(parsed.diagnostics.empty());
    EXPECT_NE(parsed.diagnostics[0].message.find("invalid <surface> width"),
              std::string::npos);
  }
}

TEST(Parser, EnforcesStyleAndScriptRestrictions) {
  const auto styled = ParseMarkup(
      "<arui><style type='text/css'>panel { gap: 1mm; }</style></arui>");
  EXPECT_FALSE(styled);
  ASSERT_FALSE(styled.diagnostics.empty());
  EXPECT_EQ(styled.diagnostics[0].message,
            "<style> does not accept attributes");

  const auto nestedStyle = ParseMarkup("<arui><style><panel/></style></arui>");
  EXPECT_FALSE(nestedStyle);
  EXPECT_NE(nestedStyle.diagnostics[0].message.find("cannot contain markup"),
            std::string::npos);

  const auto nestedScript =
      ParseMarkup("<arui><script><panel/></script></arui>");
  EXPECT_FALSE(nestedScript);
  EXPECT_NE(nestedScript.diagnostics[0].message.find("cannot contain markup"),
            std::string::npos);
}

TEST(Parser, RejectsInvalidRootUnknownNodesAndTopLevelElements) {
  EXPECT_FALSE(ParseMarkup("<surface width=\"1m\" height=\"1m\"/>"));
  auto unknown = ParseMarkup(
      "<arui><surface width=\"1m\" height=\"1m\"><slider/></surface></arui>");
  EXPECT_FALSE(unknown);
  EXPECT_NE(unknown.diagnostics[0].message.find("slider"), std::string::npos);
  auto top = ParseMarkup("<arui><metadata/></arui>");
  EXPECT_FALSE(top);
  EXPECT_NE(top.diagnostics[0].message.find("metadata"), std::string::npos);
}

TEST(Parser, RejectsInvalidAttributesExternalScriptsAndMalformedXml) {
  auto attribute = ParseMarkup("<arui><surface width=\"1m\" height=\"1m\"><row "
                               "bogus=\"x\"/></surface></arui>");
  EXPECT_FALSE(attribute);
  EXPECT_NE(attribute.diagnostics[0].message.find("bogus"), std::string::npos);
  auto external = ParseMarkup("<arui><script src=\"./test.js\"/></arui>");
  EXPECT_FALSE(external);
  EXPECT_NE(external.diagnostics[0].message.find("./test.js"),
            std::string::npos);
  auto malformed = ParseMarkup("<arui><surface></arui>");
  EXPECT_FALSE(malformed);
  EXPECT_NE(malformed.diagnostics[0].message.find("XML syntax error"),
            std::string::npos);
  EXPECT_GT(malformed.diagnostics[0].column, 0u);
}

TEST(Parser, MarkupRoundTripsAsAruiDocument) {
  const auto parsed = ParseMarkup(R"(<arui><style>panel { gap: 1mm; }</style>
    <surface width="40cm" height="25cm"><text>Hello</text></surface></arui>)");
  ASSERT_TRUE(parsed);
  const auto reparsed = ParseMarkup(SerializeMarkup(parsed.value));
  ASSERT_TRUE(reparsed) << reparsed.diagnostics[0].message;
  ASSERT_EQ(reparsed.value.surfaces.size(), 1u);
  EXPECT_EQ(
      *reparsed.value.surfaces[0].children[0].GetAttribute<std::string>("text"),
      "Hello");
}

TEST(Parser, StyleSheetRoundTripsUnchanged) {
  const auto parsed = ParseStyles("surface { width: 50cm; painter: ascii; }");
  ASSERT_TRUE(parsed);
  const auto reparsed = ParseStyles(SerializeStyles(parsed.value));
  ASSERT_TRUE(reparsed);
  EXPECT_EQ(reparsed.value, parsed.value);
}

TEST(Parser, ResolvesClassFillToTypedColor) {
  const auto parsed = ParseARUI(R"(<arui>
    <style>.red { fill: #ff000080; }</style>
    <surface width="20cm" height="10cm"><panel class="red"/></surface>
  </arui>)");
  ASSERT_TRUE(parsed) << parsed.diagnostics.front().message;
  const auto &properties =
      OnlySurface(parsed).children.at(0).style.painterProperties;
  const auto *color = std::get_if<Color>(&properties.at("fill"));
  ASSERT_NE(color, nullptr);
  EXPECT_FLOAT_EQ(color->rgba.r, 1.0F);
  EXPECT_FLOAT_EQ(color->rgba.g, 0.0F);
  EXPECT_FLOAT_EQ(color->rgba.b, 0.0F);
  EXPECT_FLOAT_EQ(color->rgba.a, 128.0F / 255.0F);
}

TEST(Parser, RejectsNamedAndMalformedFillColors) {
  EXPECT_FALSE(ParseARUI(R"(<arui><style>.bad { fill: red; }</style>
    <surface width="1m" height="1m"><panel class="bad"/></surface></arui>)"));
  EXPECT_FALSE(ParseARUI(R"(<arui><style>.bad { fill: #1234; }</style>
    <surface width="1m" height="1m"><panel class="bad"/></surface></arui>)"));
}
