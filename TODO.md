* [ ] **Build docs**
  * [x] README
    * [x] Project overview
    * [x] Build instructions
    * [x] Supported platforms
  * [ ] Usage
    * [ ] CLI invocation patterns
    * [ ] Input/output conventions
    * [ ] Exit codes
    * [ ] Environment variables
  * [ ] Examples
    * [ ] Control flow examples
    * [ ] Functions & parameters
    * [X] Interop with C
  * [ ] System
    * [ ] Architecture overview
    * [X] Compiler pipeline (Lexer -> Parser -> LLVM)
    * [ ] Error model
    * [ ] Design constraints & non-goals
* [ ] **Frontend**
  * [ ] Compiler options
    * [x] `--verbose`
    * [x] `--version`
    * [X] `-G,--object`
      * [ ] Emit object file
      * [ ] Target-specific formats
    * [X] `-S,--emit-llvm`
    * [ ] `-O,--opt-level`
    * [X] `-o,--output`
    * [ ] `--target`
  * [x] Graceful Error Handling
    * [x] Structured error types
    * [ ] Error codes
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
      * [ ] Line/column tracking
      * [ ] Source snippet highlighting
  * [ ] Unicode handling
  * [ ] Keyword vs identifier disambiguation
* [ ] **Parser**
  * [x] Expression parsing
  * [ ] Statement parsing
  * [ ] Operator precedence
  * [ ] Error recovery
  * [ ] AST generation
    * [ ] Typed AST
    * [ ] Source span metadata
* [ ] **Semantic Analysis**
  * [ ] Symbol table
  * [ ] Scope resolution
  * [ ] Type checking
  * [ ] Implicit casts
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
    * [x] Variable variable types
    * [ ] Implicit matching
    * [ ] Lifetime tracking
    * [ ] Stack vs heap allocation
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
  * [ ] Platform ABIs
    * [X] x86_64 System V
    * [ ] Windows x64
* [ ] **Optimizer**
  * [ ] Constant folding
  * [ ] Dead code elimination
  * [ ] Common subexpression elimination
  * [ ] Inlining
  * [ ] Control-flow graph generation
  * [ ] SSA form (optional)
* [ ] **Tooling**
  * [ ] Debug symbols
  * [ ] Source maps
  * [ ] Compiler tests
    * [ ] Lexer tests
    * [ ] Parser tests
    * [ ] Codegen tests
  * [ ] Fuzzing