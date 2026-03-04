* [ ] Documentation
  * [x] README
    * [x] Project overview
    * [x] Build instructions
    * [x] Supported platforms
  * [ ] User Guide
    * [ ] Installation
    * [ ] CLI usage
      * [ ] Invocation patterns
      * [ ] Flags & options
      * [ ] Input/output conventions
      * [ ] Exit codes
      * [ ] Environment variables
    * [ ] Language basics
    * [ ] Standard library overview
    * [ ] FFI / C interop
    * [ ] Build modes (debug/release)
  * [ ] Examples
    * [ ] Hello world
    * [ ] Control flow
    * [ ] Functions & parameters
    * [ ] Structs / types
    * [x] Interop with C
    * [ ] Larger sample program
  * [ ] Compiler Internals
    * [ ] Architecture overview
    * [x] Compiler pipeline (Lexer → Parser → LLVM)
    * [ ] AST structure
    * [ ] Type system design
    * [ ] Error model
    * [ ] Code generation strategy
    * [ ] Optimization passes
    * [ ] Design constraints
    * [ ] Non-goals
  * [ ] Contributor Guide
    * [ ] Project layout
    * [ ] How to build locally
    * [ ] How to run tests
    * [ ] Code style guidelines
    * [ ] Adding a new language feature
  * [ ] Language Reference
    * [ ] Grammar (EBNF)
    * [ ] Type rules
    * [ ] Operator precedence table
    * [ ] Memory model
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
    * [ ] Context-binded error handling
      * [X] Line/column tracking
      * [ ] Source snippet highlighting
  * [ ] Unicode handling
  * [ ] Keyword vs identifier disambiguation
* [ ] **Parser**
  * [x] Expression parsing
  * [x] Statement parsing
  * [ ] Error recovery
  * [X] AST generation
    * [x] Typed AST
    * [x] Source span metadata
* [ ] **Semantic Analysis**
  * [x] Symbol table
  * [x] Scope resolution
  * [X] Type checking
  * [X] Implicit casts
  * [ ] Function signature validation
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
    * [ ] Structs
    * [ ] Generics
    * [x] Variable variable types
    * [X] Implicit matching
    * [ ] Stack vs heap allocation (Needs to be worked)
  * [ ] Memory
    * [ ] Stack frame layout
    * [ ] Alignment rules
    * [ ] ABI compliance
      * [X] SystemV
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
    * [ ] Lexer
    * [ ] Ast
    * [ ] Analyzer
    * [ ] Generator 
  * [ ] Fuzzing
