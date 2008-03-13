#ifndef __WRITER_H
#define __WRITER_H

#include <com.h>

#ifdef __cplusplus
extern "C" {
#endif


writer_t
writer_new(command_t command);

void
writer_free(writer_t* writer);

void
writer_write(writer_t writer);

void
writer_wait(writer_t writer);

#ifdef __cplusplus
}
#endif

#endif /* !__WRITER_H */
