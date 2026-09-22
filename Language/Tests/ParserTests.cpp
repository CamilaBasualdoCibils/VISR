#include "ARUI/Language/Parser.hpp"
#include "ARUI/Language/Serializer.hpp"
#include <iostream>
#include <string_view>
#include <gtest/gtest.h>
using namespace ARUI::Language;
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
//TEST
//  constexpr std::string_view markup =
//      R"(<app id="music"><templates><template id="default"><panel><text value="$track.title"/><button>Play &amp; pause</button></panel></template></templates></app>)";
//  auto document = ParseMarkup(markup);
//  if (!document || document.value.root.name != "app" ||
//      document.value.root.attributes.at(0).value != "music") {
//    std::cerr << "markup parse failed\n";
//    return 1;
//  }
//  auto markupRoundTrip = ParseMarkup(SerializeMarkup(document.value));
//  if (!markupRoundTrip || markupRoundTrip.value != document.value) {
//    std::cerr << "markup round trip failed\n";
//    return 1;
//  }
//  if (ParseMarkup("<app><panel></app>")) {
//    std::cerr << "mismatched markup accepted\n";
//    return 1;
//  }
//
//  constexpr std::string_view styles = R"(/* surface */
//#player {
//  width: 50cm;
//  surface: cylinder { radius: 80cm; arc: 60deg; };
//  follow: spring(120, 20);
//}
//button:hover { transform: translate-z(3mm); }
//)";
//  auto sheet = ParseStyles(styles);
//  if (!sheet || sheet.value.rules.size() != 2 ||
//      sheet.value.rules.at(0).declarations.size() != 3 ||
//      sheet.value.rules.at(0).declarations.at(1).value !=
//          "cylinder { radius: 80cm; arc: 60deg; }") {
//    std::cerr << "style parse failed\n";
//    return 1;
//  }
//  auto styleRoundTrip = ParseStyles(SerializeStyles(sheet.value));
//  if (!styleRoundTrip || styleRoundTrip.value != sheet.value) {
//    std::cerr << "style round trip failed\n";
//    return 1;
//  }
//  auto invalid = ParseStyles("panel { width: 2cm;");
//  if (invalid || invalid.diagnostics.empty() ||
//      invalid.diagnostics.at(0).line != 1) {
//    std::cerr << "missing style diagnostic\n";
//    return 1;
//  }
//  return 0;
//}
