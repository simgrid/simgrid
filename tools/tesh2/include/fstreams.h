#ifndef __FSTREAMS_H
#define __FSTREAMS_H

#include <com.h>

#ifdef __cplusplus
extern "C" {
#endif

fstreams_t
fstreams_new(int capacity, fn_finalize_t fn_finalize);

int
fstreams_exclude(fstreams_t fstreams, excludes_t excludes);

int 
fstreams_contains(fstreams_t fstreams, fstream_t fstream);

int
fstreams_add(fstreams_t fstreams, fstream_t fstream);

int
fstreams_free(void** fstreamsptr);

int
fstreams_get_size(fstreams_t fstreams);

int
fstreams_is_empty(fstreams_t fstreams);

int 
fstreams_contains(fstreams_t fstreams, fstream_t fstream);

int
fstreams_load(fstreams_t fstreams);



#ifdef __cplusplus
}
#endif


#endif /* !__FSTREAMS_H */
