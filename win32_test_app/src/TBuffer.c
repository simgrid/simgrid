#include <TBuffer.h>

/* struct s_Buffer connected functions. */

/* Constructs an new buffer.
 * If successful, the function returns a pointer to 
 * the new buffer. Otherwise, the function returns
 * NULL. 
 */
Buffer_t Buffer_new(void)
{
  Buffer_t buffer = (Buffer_t) calloc(1, sizeof(s_Buffer_t));

  if (NULL == buffer) {
    setErrno(E_BUFFER_ALLOCATION_FAILED);
    return NULL;
  }

  buffer->data = (char *) calloc(1, DEFAULT_Buffer_CAPACITY);


  if (NULL == buffer->data) {
    Buffer_free(buffer);
    setErrno(E_BUFFER_DATA_ALLOCATION_FAILED);
    return NULL;
  }

  buffer->capacity = DEFAULT_Buffer_CAPACITY;
  Buffer_clear(buffer);
  return buffer;
}

/* Clears the buffer (this function don't destroy it,
 * see Buffer_free function.). 
 */
void Buffer_clear(Buffer_t buffer)
{
  /* must be a valid buffer. */
  ASSERT_VALID_Buffer(buffer);

  buffer->size = 0;
  buffer->data[0] = '\n';
  buffer->data[1] = '\0';
}

/* Appends a string in the buffer. If successful, 
 * the function returns true. Otherwise the function
 * returns false.
 */
bool Buffer_append(Buffer_t buffer, char *str)
{
  size_t len = strlen(str);
  size_t capacity_needed = buffer->size + len + 1;
  size_t capacity_available = buffer->capacity - buffer->size;

  /* must be a valid buffer. */
  ASSERT_VALID_Buffer(buffer);
  /* must be a valid string. */
  ASSERT_NOT_NULL(str);

  if (capacity_available < capacity_needed) {
    buffer->data = (char *) realloc(buffer->data, capacity_needed);

    if (NULL == buffer->data) {
      setErrno(E_Buffer_DATA_REALLOCATION_FAILED);
      return false;
    }

    buffer->capacity = capacity_needed;
  }

  strcpy(buffer->data + buffer->size, str);
  buffer->size += len; /*  + 1 */ ;

  return true;
}

/* 
 * Removes all the linefeed from the buffer. 
 */
void Buffer_chomp(Buffer_t buffer)
{
  /* must be a valid buffer. */
  ASSERT_VALID_Buffer(buffer);

  while ((buffer->data[buffer->size - 1] == '\n')
         || (buffer->data[buffer->size - 1] == '\r')) {
    buffer->data[buffer->size - 1] = '\0';

    if (buffer->size)
      (buffer->size)--;
  }
}

/* 
 * Destroy the buffer. 
 */
void Buffer_free(Buffer_t buffer)
{
  if (NULL == buffer)
    return;

  if (NULL != buffer->data)
    free(buffer->data);

  if (NULL != buffer)
    free(buffer);
}

/* 
 * This function returns true is the buffer is empty.
 * Otherwrise the function returns false.
 */
bool Buffer_empty(Buffer_t buffer)
{
  /* must be a valid buffer. */
  ASSERT_VALID_Buffer(buffer);
  return (buffer->size) == 0;
}
