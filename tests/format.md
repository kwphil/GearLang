Just start by working on lexer

# Lexer

```json
"type": "lexer"
"return": "<int>" // The expected return code, if it is != 0, just match for the code and exit
"match": [ // Only used when return == 0, just loop through and match the output on --dump-tokens option
    [ "extern", 2, "Keyword" ],
    // ...
]
```
