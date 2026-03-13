* [ ] **Frontend**
  * [ ] Compiler options
    * [x] `--verbose`
    * [x] `--version`
    * [X] `-c,--object`
      * [x] Emit object file
      * [ ] Target-specific formats
    * [X] `-S,--emit-llvm`
    * [X] `--dump-ast`
    * [X] `--dump-tokens`
    * [ ] `--dump-analyzer`
    * [ ] `-O,--opt-level`
    * [X] `-o,--output`
    * [ ] `--target`
  * [x] Graceful Error Handling
    * [x] Structured error types
    * [X] Error codes
    * [ ] Error recovery strategy
* [ ] **Lexer**
  * [x] Basic functionality
    * [x] Token stream generation
    * [x] Whitespace & comments
  * [x] Error handling
    * [x] Graceful
    * [ ] Colored
      * [ ] ANSI support
      * [ ] Disable color flag
    * [X] Context-binded error handling
      * [X] Line/column tracking
      * [X] Source snippet highlighting
  * [X] Keyword vs identifier disambiguation
* [X] **Parser**
  * [x] Expression parsing
  * [x] Statement parsing
  * [X] Error recovery
  * [X] AST generation
    * [x] Typed AST
    * [x] Source span metadata
* [ ] **Semantic Analysis**
  * [x] Symbol table
  * [x] Scope resolution
  * [X] Type checking
  * [X] Implicit casts
  * [X] Function signature validation
  * [ ] Unreachable code detection
* [ ] **Code Generation**
  * [x] Simple Expressions
    * [x] Arithmetic
    * [x] Comparisons
  * [ ] Branching
    * [x] If statements
    * [x] If/Else
    * [ ] Match/Case
      * [ ] Exhaustiveness checking
      * [ ] Pattern binding
  * [x] Functions
    * [x] Main function
    * [x] Calling functions
    * [x] Parameters
    * [x] Non-exit return
    * [X] Recursion
    * [ ] Tail-call optimization hooks
  * [ ] Variables
    * [X] Basic types
    * [X] Pointers
    * [X] Structs
    * [ ] Generics
    * [x] Variable variable types
    * [X] Implicit matching
* [ ] **Compatibility**
  * [x] Basic C Functions
    * [x] C Runtime
    * [x] Basic data
    * [x] C Strings
    * [x] Variadic Args
    * [ ] Header-based bindings
  * [ ] Basic C++ Functionality
    * [ ] Common Name Mangling
    * [ ] Extern "C++" support
    * [ ] Simple class interop
* [ ] **Optimizer**
  * [ ] Constant folding
  * [ ] Dead code elimination
  * [ ] Common subexpression elimination
  * [ ] Inlining
  * [ ] Control-flow graph generation
* [ ] **Tooling**
  * [ ] Debug symbols
  * [ ] Source maps
  * [ ] Test suite
    * [ ] Documentation for adding tests
    * [X] Lexer
    * [ ] Ast
    * [ ] Analyzer
    * [ ] Generator 
  * [ ] Fuzzing
