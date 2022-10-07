#include "serialize_prealloc.h"

#include <R.h>
#include <Rinternals.h>
#include <stdlib.h> // for NULL
#include <R_ext/Rdynload.h>

extern SEXP serialize_prealloc_(SEXP t_object, SEXP t_object_size, SEXP t_prealloc, SEXP t_trace) {
    return serialize_prealloc(t_object, Rf_asReal(t_object_size), Rf_asInteger(t_prealloc), Rf_asInteger(t_trace));
}

static const R_CallMethodDef CallEntries[] = {
    {"_serialize_prealloc_", (DL_FUNC) &serialize_prealloc_, 4},
    {NULL, NULL, 0}
};

void R_init_serializeprealloc(DllInfo *dll)
{
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE);
}
