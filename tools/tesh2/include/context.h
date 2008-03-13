#ifndef _CONTEXT_H
#define _CONTEXT_H

#include <com.h>

#ifdef __cplusplus
extern "C" {
#endif

context_t
context_new(void);

void
context_clear(context_t context);

void
context_reset(context_t context);

void
context_input_write(context_t context, const char* buffer);

void
context_ouput_read(context_t context, const char* buffer);



context_t
context_dup(context_t context);

void
context_free(context_t* context);

#ifdef __cplusplus
extern }
#endif

#endif /* !_CONTEXT_H */
