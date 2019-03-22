// XXX This is for restructuring tests only:
#ifdef SWAPOUT
#define SWAPOUT_FUNCTION(f)  __CONCAT(orig,f)
#else
#define SWAPOUT_FUNCTION(f)  f
#endif

