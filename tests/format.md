The idea behind these is that it uses test.gear and test.json, one creating an input, and the other defining how it should output.

# Lexer

```json
"type": "lexer"
"return": "<int>" // The expected return code, if it is != 0, just match for the code and exit
"match": [ // Only used when return == 0, just loop through and match the output on --dump-tokens option
    [ "extern", 2, "Keyword" ],
    // ...
]
```

# AST

I'll release docs for how each node should be structured soon

```json
"type": "ast"
"return": "<int>" // The expected return code. If != 0, doesn't worry about matching
"match": [
  { 
    "type": 
    "Let", 
    "var":{ 
      "type": 
      "ExprVar", 
      "name": 
      "a" 
    },
    "type": undefined,
    "expr":{ 
      "type": "ExprIntLit" 
      "value": 3
    } 
  }
]

```
