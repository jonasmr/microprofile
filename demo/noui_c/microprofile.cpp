// Microprofile implementation _requires_ c++.

#define MICROPROFILE_MINIZ 1
#if MICROPROFILE_MINIZ
#include "miniz.c"
#endif

#define MICROPROFILE_IMPL
#include "microprofile.h"

