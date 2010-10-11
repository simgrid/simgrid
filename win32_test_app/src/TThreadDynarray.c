
#include <TThreadDynarray.h>

/*
 * Constructs a ThreadDynarray with the specified capacity.
 */
ThreadDynarray_t ThreadDynarray_new(unsigned long capacity)
{
  ThreadDynarray_t ptr = calloc(1, sizeof(s_ThreadDynarray_t));

  ptr->count = 0;
  ptr->capacity = capacity;

  memset(&(ptr->cs), 0, sizeof(CRITICAL_SECTION));
  InitializeCriticalSection(&(ptr->cs));
  ptr->is_locked = false;

  if (capacity)
    ptr->threads =
        (ThreadEntry_t) calloc(capacity, sizeof(s_ThreadEntry_t));
  else
    ptr->threads = NULL;

  return ptr;
}

/* 
 * Destroy the ThreadDynarray 
 */
void ThreadDynarray_destroy(ThreadDynarray_t ptr)
{
  ThreadDynarray_clear(ptr);
  DeleteCriticalSection(&(ptr->cs));
  free(ptr);
  ptr = NULL;
}

/*
 * Returns an const pointer to entry pointed to by index.
 */
ThreadEntry_t const ThreadDynarray_at(ThreadDynarray_t ptr,
                                      unsigned long index)
{
  ThreadEntry_t __entry;
  ThreadDynarray_lock(ptr);
  __entry = &(ptr->threads)[index];
  ThreadDynarray_unlock(ptr);
  return __entry;
}

/*
 * Fill the content of the entry addressed by __entry with the content
 * of the entry pointed to by index.
 */
void ThreadDynarray_get(ThreadDynarray_t ptr, unsigned long index,
                        ThreadEntry_t const __entry)
{
  ThreadDynarray_lock(ptr);
  ::memcpy(__entry, ThreadDynarray_at(ptr, index),
           sizeof(s_ThreadEntry_t));
  ThreadDynarray_unlock(ptr);
}

/* 
 * Fill the content of the entry pointed to by index with the content of
 * the entry addressed by __entry.
 */
void ThreadDynarray_set(ThreadDynarray_t ptr, unsigned long index,
                        ThreadEntry_t const __entry)
{

  ThreadDynarray_lock(ptr);
  memcpy(ThreadDynarray_at(ptr, index), __entry, sizeof(s_ThreadEntry_t));
  ThreadDynarray_unlock(ptr);
}

/*
 * Returns a const pointer to the first entry.
 */
ThreadEntry_t const ThreadDynarray_getFront(ThreadDynarray_t ptr)
{
  ThreadEntry_t __entry;
  ThreadDynarray_lock(ptr);
  __entry = ThreadDynarray_at(ptr, 0);
  ThreadDynarray_unlock(ptr);
  return __entry;
}

/*
 * Returns a const pointer to the last entry.
 */
ThreadEntry_t const ThreadDynarray_getBack(ThreadDynarray_t ptr)
{
  ThreadEntry_t __entry;
  ThreadDynarray_lock(ptr);
  __entry = ThreadDynarray_at(ptr, ptr->count - 1);;
  ThreadDynarray_unlock(ptr);
  return __entry;
}

/*
 * Inserts a copy of __entry at the front
 */
void ThreadDynarray_pushFront(ThreadDynarray_t ptr,
                              ThreadEntry_t const __entry)
{
  ThreadDynarray_lock(ptr);

  if (!ThreadDynarray_getCapacityAvailable(ptr))
    ThreadDynarray_resize(ptr);

  ptr->count++;
  ThreadDynarray_move(ptr, 1, ThreadDynarray_getLowerBound(ptr),
                      ThreadDynarray_getUpperBound(ptr));
  ThreadDynarray_set(ptr, ThreadDynarray_getLowerBound(ptr), __entry);

  ThreadDynarray_unlock(ptr);
}

/*
 * Appends a copy of __entry to the end.
 */
void ThreadDynarray_pushBack(ThreadDynarray_t ptr,
                             ThreadEntry_t const __entry)
{
  ThreadDynarray_lock(ptr);

  if (!ThreadDynarray_getCapacityAvailable(ptr))
    ThreadDynarray_resize(ptr);

  ptr->count++;
  ThreadDynarray_set(ptr, ThreadDynarray_getUpperBound(ptr), __entry);

  ThreadDynarray_unlock(ptr);
}


/* 
 * Inserts __entry at the position pointed to by index.
 */
void ThreadDynarray_insert(ThreadDynarray_t ptr, unsigned long index,
                           ThreadEntry_t const __entry)
{
  ThreadDynarray_lock(ptr);

  if (!ThreadDynarray_getCapacityAvailable(ptr))
    ThreadDynarray_resize(ptr);

  ThreadDynarray_move(ptr, index + 1, index, ptr->count - index);
  ptr->count++;
  ThreadDynarray_set(ptr, index, __entry);

  ThreadDynarray_unlock(ptr);
}

/*
 * Deletes the entry pointed to by index. If __entry is not NULL the
 * fuction saves the entry threads at this address before.
 */
void ThreadDynarray_erase(ThreadDynarray_t ptr, unsigned long index,
                          ThreadEntry_t const __entry)
{

  ThreadDynarray_lock(ptr);

  if (__entry)
    ThreadDynarray_set(ptr, index, __entry);

  if (index != ThreadDynarray_getUpperBound(ptr))
    ThreadDynarray_move(ptr, index, index + 1, (ptr->count - (index + 1)));

  ptr->count--;

  ThreadDynarray_unlock(ptr);
}

/*
 * Find the first entry with the same content of the entry addressed by
 * __entry.The function returns the index of the founded entry, -1 if
 * no entry is founded.
 */
long ThreadDynarray_getIndex(ThreadDynarray_t ptr,
                             ThreadEntry_t const __entry)
{

  unsigned long i;
  ThreadDynarray_lock(ptr);

  for (i = 0; i < ptr->count; i++) {
    if (ThreadDynarray_compare(ptr, i, __entry)) {
      ThreadDynarray_unlock(ptr);
      return i;
    }
  }

  ThreadDynarray_unlock(ptr);
  return -1;
}

/* 
 * Returns true if the entry exist.
 */
bool ThreadDynarray_exist(ThreadDynarray_t ptr,
                          ThreadEntry_t const __entry)
{
  bool exist;

  ThreadDynarray_lock(ptr);
  exist = (-1 != ThreadDynarray_getIndex(ptr, __entry));
  ThreadDynarray_unlock(ptr);
  return exist;
}

/* Deletes the first entry with the same content of the entry addressed
 * by __entry.The function returns true if the entry is deleted, false
 * if no entry is founded.
 */
bool ThreadDynarray_remove(ThreadDynarray_t ptr,
                           ThreadEntry_t const __entry)
{
  /* assert(!empty(ptr)); */

  long __index;
  ThreadDynarray_lock(ptr);
  __index = ThreadDynarray_getIndex(ptr, __entry);

  if (__index == -1) {
    ThreadDynarray_unlock(ptr);
    return false;
  }

  ThreadDynarray_set(ptr, (unsigned long) __index, NULL);
  ThreadDynarray_unlock(ptr);
  return true;
}

/*
 * Erase all elements of the self.
 */
void ThreadDynarray_clear(ThreadDynarray_t ptr)
{
  ThreadDynarray_lock(ptr);

  if (ptr->threads) {
    free(ptr->threads);
    ptr->threads = NULL;
  }

  ptr->count = 0;
  ptr->capacity = 0;
  ThreadDynarray_unlock(ptr);
}

/*
 * Resets entry count to zero.
 */
void ThreadDynarray_reset(ThreadDynarray_t ptr)
{
  ThreadDynarray_lock(ptr);
  ptr->count = 0;
  ThreadDynarray_unlock(ptr);
}

/*
 * Moves count elements from src index to dst index.
 */
void ThreadDynarray_move(ThreadDynarray_t ptr, const unsigned long dst,
                         const unsigned long src, unsigned long count)
{
  ThreadDynarray_lock(ptr);

  if (ptr->count)
    memmove(ThreadDynarray_at(ptr, dst), ThreadDynarray_at(ptr, src),
            count * sizeof(s_ThreadEntry_t));

  ThreadDynarray_unlock(ptr);
}

/* Compare the content of the entry pointed to by index with the content of
 * the entry addressed by __entry. The function returns true if the contents
 * are same.
 */
bool ThreadDynarray_compare(ThreadDynarray_t ptr,
                            const unsigned long index,
                            ThreadEntry_t const __entry)
{
  bool are_equals;
  ThreadDynarray_lock(ptr);
  are_equals =
      (!memcmp
       (ThreadDynarray_at(ptr, index), __entry, sizeof(s_ThreadEntry_t)));
  ThreadDynarray_unlock(ptr);
  return are_equals;
}

/*
 * Returns a reference to a new ThreadDynarray new set is a clone of the self.
 */
ThreadDynarray_t ThreadDynarray_clone(ThreadDynarray_t ptr)
{
  ThreadDynarray_t new_ptr;
  ThreadDynarray_lock(ptr);
  ptr = ThreadDynarray_new(ptr->capacity);

  if (ptr->count) {
    memcpy(new_ptr->threads, ptr->threads,
           ptr->count * sizeof(s_ThreadEntry_t));
    new_ptr->count = ThreadDynarray_getCount(ptr);
  }
  ThreadDynarray_unlock(ptr);
  return new_ptr;
}

/*
 * Extends the capacity when the container is full.
 */
void ThreadDynarray_resize(ThreadDynarray_t ptr)
{
  ThreadDynarray_lock(ptr);

  ptr->capacity = (!ptr->capacity) ? 1 : (ptr->count << 1);
  ptr->threads =
      (ThreadEntry_t) realloc(ptr->threads,
                              ptr->capacity * sizeof(s_ThreadEntry_t));

  ThreadDynarray_unlock(ptr);
}


/*
 * Returns the number of elements.
 */
unsigned long ThreadDynarray_getCount(ThreadDynarray_t ptr)
{
  unsigned count;
  ThreadDynarray_lock(ptr);
  count = ptr->count;
  ThreadDynarray_unlock(ptr);
  return count;
}

/*
 * Returns the current storage capacity of the ThreadDynarray. This is guaranteed
 * to be at least as large as count().
 */
unsigned long ThreadDynarray_getCapacity(ThreadDynarray_t ptr)
{
  unsigned capacity;
  ThreadDynarray_lock(ptr);
  capacity = ptr->capacity;
  ThreadDynarray_unlock(ptr);
  return capacity;
}


/*
 * Returns upper bound of self (max index).
 */
unsigned long ThreadDynarray_getUpperBound(ThreadDynarray_t ptr)
{
  unsigned long upper_bound;
  ThreadDynarray_lock(ptr);
  upper_bound = (ptr->count - 1);
  ThreadDynarray_unlock(ptr);
  return upper_bound;
}

/*
 * Returns lower bound of self (always zero).
 */
unsigned long ThreadDynarray_getLowerBound(ThreadDynarray_t ptr)
{
  return 0;
}

/*
 * Returns the size of the elements.
 */
unsigned long ThreadDynarray_getElementSize(ThreadDynarray_t ptr)
{
  return sizeof(s_ThreadEntry_t);
}

/*
 * Returns true if the size of self is zero.
 */
bool ThreadDynarray_isEmpty(ThreadDynarray_t ptr)
{
  bool is_empty;
  ThreadDynarray_lock(ptr);
  is_empty = (ptr->count == 0);
  ThreadDynarray_unlock(ptr);
  return is_empty;
}

/*
 * Returns true if capacity available.
 */
bool ThreadDynarray_getCapacityAvailable(ThreadDynarray_t ptr)
{
  bool capacity_available;
  ThreadDynarray_lock(ptr);
  capacity_available = (ptr->capacity > ptr->count);
  ThreadDynarray_unlock(ptr);
  return capacity_available;
}

/*
 * Returns true if the container is full.
 */
bool ThreadDynarray_is_full(ThreadDynarray_t ptr)
{
  bool is_full;
  ThreadDynarray_lock(ptr);
  is_full = (!ThreadDynarray_isEmpty(ptr)
             && !ThreadDynarray_getCapacityAvailable(ptr));
  ThreadDynarray_unlock(ptr);
  return is_full;
}

/* 
 * Assignement.
 */
ThreadDynarray_t ThreadDynarray_assign(ThreadDynarray_t src,
                                       ThreadDynarray_t dst)
{
  ThreadDynarray_lock(src);
  ThreadDynarray_lock(dst);

  if (src != dst) {
    ThreadDynarray_clear(dst);

    if (src->count) {
      dst->count = src->count;
      dst->capacity = src->capacity;
      dst->threads =
          (ThreadEntry_t) malloc(src->capacity * sizeof(s_ThreadEntry_t));
      memcpy(dst->threads, src->threads,
             src->count * sizeof(s_ThreadEntry_t));
    }
  }
  ThreadDynarray_unlock(src);
  ThreadDynarray_unlock(dst);

  return dst;
}

/* 
 * Returns true if the dynamic arrays are equal.
 */
bool ThreadDynarray_areEquals(ThreadDynarray_t ptr1, ThreadDynarray_t ptr2)
{
  bool are_equals;

  ThreadDynarray_lock(ptr1);
  ThreadDynarray_lock(ptr2);

  are_equals = (ptr1->count == ptr2->count &&
                ptr1->capacity == ptr2->capacity &&
                !memcmp(ptr2->threads, ptr1->threads, ptr1->capacity)
      );

  ThreadDynarray_unlock(ptr1);
  ThreadDynarray_unlock(ptr2);

  return are_equals;
}

/* 
 * Returns true if the dynamic arrays are not equal.
 */
ThreadDynarray_areNotEquals(ThreadDynarray_t ptr1, ThreadDynarray_t ptr2)
{
  return !ThreadDynarray_areEquals(ptr1, ptr2);
}

void ThreadDynarray_lock(ThreadDynarray_t ptr)
{
  if (!ptr->is_locked) {
    EnterCriticalSection(&(ptr->cs));
    ptr->is_locked = true;
  }
}

void ThreadDynarray_unlock(ThreadDynarray_t ptr)
{
  if (ptr->is_locked) {
    LeaveCriticalSection(&(ptr->cs));
    ptr->is_locked = false;
  }
}
