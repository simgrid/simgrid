#ifndef __FSTREAM_H
#define __FSTREAM_H

#include <com.h>

#ifdef __cplusplus
extern "C" {
#endif

fstream_t
fstream_new(const char* directory, const char* name);

int
fstream_open(fstream_t fstream);

int
fstream_close(fstream_t fstream);

int
fstream_free(void** fstreamptr);

void
fstream_parse(fstream_t fstream, unit_t unit);

#ifdef __cplusplus
}
#endif


#endif /*! __FSTREAM_H */
