//
// For non DEBUG build all the printf statments can be removed.
// The compiler should recognize that the empty statement is unnecessary 
// and completely optimize it away, leaving no trace of the debug message 
// in the final compiled binary.
//
// See add_definitions(-DDEBUG_BUILD) in CMakeLists.txt file
//

#if DEBUG_BUILD
#define DEBUG_printf(...) printf(__VA_ARGS__)
#else
#define DEBUG_printf(...) ((void)0)
#endif
