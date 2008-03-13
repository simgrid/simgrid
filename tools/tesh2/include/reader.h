#ifndef __READER_H
#define __READER_H

#include <com.h>

#ifdef __cplusplus
extern "C" {
#endif



reader_t
reader_new(command_t command);

void
reader_free(reader_t* reader);

void
reader_read(reader_t reader);

void
reader_wait(reader_t reader);

#ifdef __cplusplus
}
#endif

#endif /* !__READER_H */
