// XXX This is for restructuring tests only:

#ifndef __CONCAT1
#define __CONCAT1(x, y) x##y
#endif

#ifdef SWAPOUT
#define SWAPOUT_FUNCTION(f)  __CONCAT1(orig,f)
#define SWAPOUT_FUNCTION_X(f)  __CONCAT1(_inactive_,f)
#else
#define SWAPOUT_FUNCTION(f)  f
#define SWAPOUT_FUNCTION_X(f) f
#endif


