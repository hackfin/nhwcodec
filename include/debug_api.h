// XXX This is for restructuring tests only:
#ifdef SWAPOUT
#define SWAPOUT_FUNCTION(f)  __CONCAT(orig,f)
#define SWAPOUT_FUNCTION_X(f)  __CONCAT(_inactive_,f)
#else
#define SWAPOUT_FUNCTION(f)  f
#define SWAPOUT_FUNCTION_X(f) f
#endif


