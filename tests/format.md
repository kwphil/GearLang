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

Each node contains at least a "type" index, usually with other information to come with it

Each node has a description of an expected format

If you don't see one, looks like YOU get to create the format

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

## If

```json
{
  "type": "If",
  "cond": <pExpr>,
  "expr": <pExpr> 
}
```

## Else

```json
{
  "type": "Else",
  "cond": <pExpr>,
  "exprtrue": <pExpr>,
  "exprfalse": <pExpr>
}
```

## ExprOp

* `type`: `pExpr`

```json
{
  "type": "ExprOp",
  "oper": <string>,
  "left": <pExpr>,
  "right": <pExpr>
}
```

## ExprCall

* `type`: `pExpr`

```json
{
  "type": "ExprCall",
  "callee": <string>,
  "args": <array of pExpr>
}
```

## ExprAddress

* `type`: `pExpr`

```json
{
  "type": "ExprAddress",
  "var": <variable>
}
```

## ExprDeref

* `type`: `pExpr`

```json
{
  "type": "ExprDeref",
  "var": <variable>
}
```

## Argument

* `type`: `variable`

```json
{
  "type": "Argument",
  "name": <string>,
  "ty": <Sem.Type>
}
```

## Function

```json
{
  "type": "Function",
  "name": <string>,
  "ty": <Sem.Type>, // Return type
  "args": <array of Argument>,
  "block": <pExpr>,
  "is_variadic": <bool>
}
```

## ExternFn

```json
{
  "type": "ExternFn",
  "name": <string>,
  "ty": <Sem.Type>, // Return type
  "args": <array of Argument>,
  "is_variadic": <bool>,
  "no_mangle": <bool>
}
```

## ExprLitInt

* `type`: `pExpr`

```json
{
  "type": "ExprLitInt",
  "value": <long long>
}
```

## ExprLitFloat

* `type`: `pExpr`

```json
{
  "type": "ExprLitFloat",
  "value": <double>
}
```

## ExprLitString

* `type`: `pExpr`

```json
{
  "type": "ExprLitString",
  "string" <string>
}
```

## Struct

```json
{
  "type": "Struct",
  "args": <array of this.Arg>
}
```

### Arg

```json
{
  "name": <string>,
  "ty": <Sem.Type>
}
```

## Let

```json
{
  "type": "Let",
  "target": <variable>,
  "expr": <null|pExpr>
}
```

## ExprBlock

`type`: `pExpr`

```json
{
  "type": "ExprBlock",
  "nodes": <array of stmt>
}
```

## Return

```json
{
  "type": "Return",
  "expr": <null|pExpr>
}
```

## Include

```json
{
  "type": "Include",
  "lang": <string>,
  "extra": <string>, // sys/loc for C
  "file": <string>
}
```

## ExprVar

* `type`: `variable`

```json
{
  "type": "ExprVar",
  "name": <string>
}
```

## ExprStructParam

* `type`: `variable`

```json
{
  "type": "ExprStructParam",
  "struct_name": <string>,
  "param_name": <string>
}
```