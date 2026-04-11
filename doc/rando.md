This file is just dedicated to random thoughts I had about implementation

## C++ 

Working on how C is implemented, I'm planning on implementing something similar for it. 

the Clang++ compiler will not be required, but if it is needed, it may link in the middle of runtime.

I'm also thinking about having a config file (Similar to Rust's Cargo.toml), that could be used to extend to the build system on both C and C++ for linking projects.

```
[cxx]
build = "cmake"
// options for using cmake and stuff
cmake_options = { "_DEBUG": { "value": "ON", "type": "string" } } // Or whatever
```

