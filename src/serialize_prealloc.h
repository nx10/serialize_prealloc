#ifndef __SERIALIZE_PREALLOC_H__
#define __SERIALIZE_PREALLOC_H__

#define R_NO_REMAP
//#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

SEXP serialize_prealloc(SEXP t_object, R_SIZE_T t_object_size, int t_prealloc, int t_trace);

#endif // __SERIALIZE_PREALLOC_H__
