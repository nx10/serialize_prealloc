

#include "serialize_prealloc.h"

//#define USE_RINTERNALS
//#include <R.h>
//#include <Rinternals.h>
//#include <R_ext/Rdynload.h>
//#include <Rdefines.h>

typedef struct membuf_st {
    R_SIZE_T size;
    R_SIZE_T count;
    unsigned char *buf;
} *membuf_t;

#define INCR 8192 // MAXELTSIZE src/include/Defn.h
static void resize_buffer(membuf_t mb, R_SIZE_T needed)
{
    if(needed > R_XLEN_T_MAX)
	Rf_error("serialization is too large to store in a raw vector");
#ifdef LONG_VECTOR_SUPPORT
    if(needed < 10000000) /* ca 10MB */
	needed = (1+2*needed/INCR) * INCR;
    else
	needed = (R_SIZE_T)((1+1.2*(double)needed/INCR) * INCR);
#else
    if(needed < 10000000) /* ca 10MB */
	needed = (1+2*needed/INCR) * INCR;
    else if(needed < 1700000000) /* close to 2GB/1.2 */
	needed = (R_SIZE_T)((1+1.2*(double)needed/INCR) * INCR);
    else if(needed < INT_MAX - INCR)
	needed = (1+needed/INCR) * INCR;
#endif
    unsigned char *tmp = realloc(mb->buf, needed);
    if (tmp == NULL) {
	free(mb->buf); mb->buf = NULL;
	Rf_error("cannot allocate buffer");
    } else mb->buf = tmp;
    mb->size = needed;
}

static void resize_buffer_trace(membuf_t mb, R_SIZE_T needed)
{
    Rprintf("resize_buffer() size: %zu count: %zu needed: %zu", mb->size, mb->count, needed);
    resize_buffer(mb, needed);
    Rprintf(" new size: %zu\n", mb->size);
}

static void free_mem_buffer(void *data)
{
    membuf_t mb = data;
    if (mb->buf != NULL) {
	unsigned char *buf = mb->buf;
	mb->buf = NULL;
	free(buf);
    }
}

static void OutCharMem(R_outpstream_t stream, int c)
{
    membuf_t mb = stream->data;
    if (mb->count >= mb->size)
	resize_buffer(mb, mb->count + 1);
    mb->buf[mb->count++] = (char) c;
}

static void OutBytesMem(R_outpstream_t stream, void *buf, int length)
{
    membuf_t mb = stream->data;
    R_SIZE_T needed = mb->count + (R_SIZE_T) length;
#ifndef LONG_VECTOR_SUPPORT
    /* There is a potential overflow here on 32-bit systems */
    if((double) mb->count + length > (double) INT_MAX)
	error(_("serialization is too large to store in a raw vector"));
#endif
    if (needed > mb->size) resize_buffer(mb, needed);
    memcpy(mb->buf + mb->count, buf, length);
    mb->count = needed;
}

static void OutCharMem_trace(R_outpstream_t stream, int c)
{
    membuf_t mb = stream->data;
    if (mb->count >= mb->size)
	resize_buffer_trace(mb, mb->count + 1);
    mb->buf[mb->count++] = (char) c;
}

static void OutBytesMem_trace(R_outpstream_t stream, void *buf, int length)
{
    membuf_t mb = stream->data;
    R_SIZE_T needed = mb->count + (R_SIZE_T) length;
#ifndef LONG_VECTOR_SUPPORT
    /* There is a potential overflow here on 32-bit systems */
    if((double) mb->count + length > (double) INT_MAX)
	error(_("serialization is too large to store in a raw vector"));
#endif
    if (needed > mb->size) resize_buffer_trace(mb, needed);
    memcpy(mb->buf + mb->count, buf, length);
    mb->count = needed;
}

static void InitMemOutPStream(R_outpstream_t stream, membuf_t mb,
			      R_pstream_format_t type, int version,
			      SEXP (*phook)(SEXP, SEXP), SEXP pdata)
{
    mb->count = 0;
    mb->size = 0;
    mb->buf = NULL;
    R_InitOutPStream(stream, (R_pstream_data_t) mb, type, version,
		     OutCharMem, OutBytesMem, phook, pdata);
}

static void InitMemOutPStream_trace(R_outpstream_t stream, membuf_t mb,
			      R_pstream_format_t type, int version,
			      SEXP (*phook)(SEXP, SEXP), SEXP pdata)
{
    mb->count = 0;
    mb->size = 0;
    mb->buf = NULL;
    R_InitOutPStream(stream, (R_pstream_data_t) mb, type, version,
		     OutCharMem_trace, OutBytesMem_trace, phook, pdata);
}

static void InitMemOutPStreamPrealloc(R_outpstream_t stream, membuf_t mb,
			      R_pstream_format_t type, int version,
			      SEXP (*phook)(SEXP, SEXP), SEXP pdata, R_SIZE_T prealloc_size)
{
    mb->count = 0;
    mb->size = prealloc_size;
    mb->buf = malloc(prealloc_size);
    R_InitOutPStream(stream, (R_pstream_data_t) mb, type, version,
		     OutCharMem, OutBytesMem, phook, pdata);
}

static void InitMemOutPStreamPrealloc_trace(R_outpstream_t stream, membuf_t mb,
			      R_pstream_format_t type, int version,
			      SEXP (*phook)(SEXP, SEXP), SEXP pdata, R_SIZE_T prealloc_size)
{
    mb->count = 0;
    mb->size = prealloc_size;
    mb->buf = malloc(prealloc_size);
    R_InitOutPStream(stream, (R_pstream_data_t) mb, type, version,
		     OutCharMem_trace, OutBytesMem_trace, phook, pdata);
}

static SEXP CloseMemOutPStream(R_outpstream_t stream)
{
    SEXP val;
    membuf_t mb = stream->data;
    /* duplicate check, for future proofing */
#ifndef LONG_VECTOR_SUPPORT
    if(mb->count > INT_MAX)
	error(_("serialization is too large to store in a raw vector"));
#endif
    PROTECT(val = Rf_allocVector(RAWSXP, mb->count));
    memcpy(RAW(val), mb->buf, mb->count);
    free_mem_buffer(mb);
    UNPROTECT(1);
    return val;
}

SEXP serialize_prealloc(SEXP t_object, R_SIZE_T t_object_size, int t_prealloc, int t_trace)
{
    struct R_outpstream_st out;
    struct membuf_st mb;

    R_pstream_format_t type = R_pstream_binary_format;

    if (!t_trace) {
        if (t_prealloc) {
            InitMemOutPStreamPrealloc(&out, &mb, type, 0, NULL, NULL, t_object_size);
        } else {
            InitMemOutPStream(&out, &mb, type, 0, NULL, NULL);
        }
    } else {
        Rprintf("serialize_prealloc() object_size: %zu\n", t_object_size);
        if (t_prealloc) {
            InitMemOutPStreamPrealloc_trace(&out, &mb, type, 0, NULL, NULL, t_object_size);
        } else {
            InitMemOutPStream_trace(&out, &mb, type, 0, NULL, NULL);
        }
    }

    R_Serialize(t_object, &out);

    SEXP val;

    PROTECT(val = CloseMemOutPStream(&out));
    
    UNPROTECT(1); /* val */
    return val;
}