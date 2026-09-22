# Language tests

Add parser, serializer, and diagnostic tests here as those implementations arrive.

The first cases should cover:

- Nested and self-closing elements, text, quoted attributes, and escaping.
- State references such as `$track.title` and repeated collections.
- Style selectors, units, functions, and nested values such as `surface: cylinder { ... }`.
- Parse errors with source locations for unmatched tags, unterminated strings, and malformed style blocks.
- Parse → serialize → parse round trips that preserve the document's meaning.

`ParserTests.cpp` exercises the current markup and style parsers, diagnostics, and structural round trips. Run it with `ctest --test-dir build --output-on-failure` after building.
