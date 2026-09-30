// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

lexer grammar SourceLexer;

// Descriptive grammar. The Source contract owns UTF-8 diagnostics and locations.
Comment: '//' ~[\r\n]*;
Identifier: [a-zA-Z_\u0080-\u{10FFFF}] [a-zA-Z_0-9\u0080-\u{10FFFF}]*;
Number: [0-9]+ ('.' [0-9]+)?;
String: '"' ('\\' ~[\r\n] | ~["\\\r\n])* '"'
      | '\'' ('\\' ~[\r\n] | ~['\\\r\n])* '\'';
Whitespace: [ \t\r\n]+ -> skip;
Symbol: [!-/:-@\u005B-\u0060{-~];
Unknown: .;
