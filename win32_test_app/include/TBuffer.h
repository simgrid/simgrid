#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <TErrno.h>
#include <string.h>
#include <stdlib.h>

/* struct s_Buffer declaration. */
typedef struct s_Buffer {
  char *data;                   /* the buffer data.                                     */
  size_t size;                  /* the buffer size (in bytes).          */
  size_t capacity;              /* the buffer capacity (in bytes).      */
} s_Buffer_t, *Buffer_t;

/* Asserts that a s_Buffer is valid. */
#define ASSERT_VALID_Buffer(p)	( ASSERT_NOT_NULL((p)) /*&& ASSERT_NOT_NULL((p)->data)*/ )

/* The default buffer capacity (512 bytes). */
#define DEFAULT_Buffer_CAPACITY	((size_t)512)

/* struct s_buffet connected functions. */

/* Constructs an new buffer.
 * If successful, the function returns a pointer to 
 * the new buffer. Otherwise, the function returns
 * NULL. 
 */
Buffer_t Buffer_new(void);

/* Clears the buffer (this function don't destroy it,
 * see Buffer_free function). 
 */
void Buffer_clear(Buffer_t buffer);

/* Appends a string in the buffer. If successful, 
 * the function returns true. Otherwise the function
 * returns false.
 */
bool Buffer_append(Buffer_t buffer, char *str);

/* 
 * Removes all the linefeed from the buffer. 
 */
void Buffer_chomp(Buffer_t buffer);

/* 
 * Destroy the buffer. 
 */
void Buffer_free(Buffer_t buffer);

/* 
 * This function returns true is the buffer is empty.
 * Otherwrise the function returns false.
 */
bool Buffer_empty(Buffer_t buffer);




#endif                          /* #ifndef __BUFFER_H__ */
