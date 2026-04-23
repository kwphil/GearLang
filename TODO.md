* [ ] **Refactor**
  * [ ] Docs
  * [ ] Lexer
  * [ ] Type system
    * [ ] Enums
    * [ ] C ABI structs
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
    * [X] `--dump-analyzer`
    * [X] `-O,--opt-level`
    * [X] `-o,--output`
    * [ ] `--target`
  * [x] Graceful Error Handling
    * [x] Structured error types
    * [X] Error codes
    * [X] Error recovery strategy
      * [X] Parsing 
      * [X] Analyzing   
    * [x] Graceful
    * [X] Colored
      * [X] ANSI support
      * [X] Disable color flag
    * [ ] Simple syntax highlighting
* [ ] **Multi-file**
  * [X] Document multi-file usage
  * [ ] Basic modules
  * [ ] Submodules
  * [ ] Directory hierarchies
* [X] **Lexer**
  * [x] Basic functionality
    * [x] Token stream generation
    * [x] Whitespace & comments
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
  * [X] Unreachable code detection
  * [ ] Match/Case Exhaustiveness check
* [ ] **Code Generation**
  * [x] Simple Expressions
    * [x] Arithmetic
    * [x] Comparisons
  * [ ] Branching
    * [x] If statements
    * [x] If/Else
    * [ ] Match/Case
      * [ ] Pattern binding
  * [ ] Functions
    * [x] Main function
    * [x] Calling functions
    * [x] Parameters
    * [x] Non-exit return
    * [X] Recursion
    * [ ] Tail-call optimization hooks
    * [ ] C++ operator overload calls
  * [ ] Variables
    * [X] Basic types
    * [X] Pointers
    * [X] Structs
    * [ ] Generics
    * [x] Variable variable types
    * [X] Implicit matching
  * [ ] Name mangling
    * [X] Global scope functions
    * [ ] Top level modules
    * [ ] Submodules
* [ ] **Compatibility**
  * [X] Basic C Functions
    * [x] C Runtime
    * [x] Basic data
    * [x] C Strings
    * [x] Variadic Args
  * [ ] Near full C Functionality
    * [X] Header-based bindings
    * [X] Typedefs
    * [ ] Inline functions
  * [ ] Basic C++ Functionality
    * [ ] Common Name Mangling
    * [ ] Extern "C++" support
    * [ ] Simple class interop
* [ ] **Optimizer**
  * [X] Constant folding
  * [X] Dead code elimination
  * [ ] Common subexpression elimination
  * [ ] Inlining
  * [ ] Control-flow graph generation
* [ ] **Tooling**
  * [ ] Debug symbols
  * [ ] Source maps
  * [ ] Test suite
    * [ ] Documentation for adding tests
    * [X] Lexer
    * [X] Ast
    * [ ] Analyzer
    * [ ] Generator 
  * [ ] Fuzzing
