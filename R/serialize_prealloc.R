#' serializeprealloc: Experimental R object serialization.
#'
#' @docType package
#' @name serializeprealloc-package
#' @useDynLib serializeprealloc, .registration=TRUE
NULL

.onUnload <- function(libpath) {
  library.dynam.unload("serializeprealloc", libpath)
}


#' Serialize with preallocated buffer.
#'
#' @param object Object for serialization.
#'
#' @param prealloc Should preallocation be used?
#' @param trace Print traces for buffer memory allocations.
#'
#' @importFrom utils object.size
#'
#' @export
serialize_prealloc <- function(object, prealloc = TRUE, trace = FALSE) {
  .Call(`_serialize_prealloc_`, object, object.size(object), prealloc, trace)
}
