#ifndef __THREAD_DYNARRAY_H__
#define __THREAD_DYNARRAY_H__

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <TTestCaseContext.h>


typedef struct s_ThreadEntry {
  HANDLE hThread;
  DWORD threadId;
  TestCaseContext_t context;
} s_ThreadEntry_t, *ThreadEntry_t;
/*
 * s_ThreadDynarray struct declaration.
 */

typedef struct s_ThreadDynarray {
  /* threads */
  ThreadEntry_t threads;
  /* thread count */
  unsigned long count;
  /* Storage capacity */
  unsigned long capacity;
  CRITICAL_SECTION cs;
  bool is_locked;

} s_ThreadDynarray_t, *ThreadDynarray_t;

/*
 * Constructs a ThreadDynarray with the specified capacity.
 */
ThreadDynarray_t ThreadDynarray_new(unsigned long capacity);

/* 
 * Destroy the ThreadDynarray 
 */
void ThreadDynarray_destroy(ThreadDynarray_t ptr);

/*
 * Returns an const pointer THREAD_entry pointed to by index.
 */
ThreadEntry_t const ThreadDynarray_at(ThreadDynarray_t ptr,
                                      unsigned long index);

/*
 * Fill the content of the entry addressed by the __entry with the content
 * of the entry pointed to by index.
 */
void ThreadDynarray_get(ThreadDynarray_t ptr, unsigned long index,
                        ThreadEntry_t const __entry);

/* 
 * Fill the content of the entry pointed to by index with the content of
 * the entry addressed by __entry.
 */
void ThreadDynarray_set(ThreadDynarray_t ptr, unsigned long index,
                        ThreadEntry_t const __entry);

/*
 * Returns a const pointer to the first entry.
 */
ThreadEntry_t const ThreadDynarray_getFront(ThreadDynarray_t ptr);

/*
 * Returns a const pointer to the last entry.
 */
ThreadEntry_t const ThreadDynarray_getBack(ThreadDynarray_t ptr);

/*
 * Inserts a copy of __entry at the front
 */
void ThreadDynarray_pushFront(ThreadDynarray_t ptr,
                              ThreadEntry_t const __entry);

/*
 * Appends a copy of __entry to the end.
 */
void ThreadDynarray_pushBack(ThreadDynarray_t ptr,
                             ThreadEntry_t const __entry);

/* 
 * Inserts __entry at the position pointed to by index.
 */
void ThreadDynarray_insert(ThreadDynarray_t ptr, unsigned long index,
                           ThreadEntry_t const __entry);

/*
 * Deletes the entry pointed to by index. If __entry is not NULL the
 * fuction saves the entry threads at this address before.
 */
void ThreadDynarray_erase(ThreadDynarray_t ptr, unsigned long index,
                          ThreadEntry_t const __entry);

/*
 * Find the first entry with the same content of the entry addressed by
 * __entry.The function returns the index of the founded entry, -1 if
 * no entry is founded.
 */
long ThreadDynarray_getIndex(ThreadDynarray_t ptr,
                             ThreadEntry_t const __entry);

/* 
 * Returns true if the entry exist.
 */
bool ThreadDynarray_exist(ThreadDynarray_t ptr,
                          ThreadEntry_t const __entry);

/* Deletes the first entry with the same content of the entry addressed
 * by __entry.The function returns true if the entry is deleted, false
 * if no entry is founded.
 */
bool ThreadDynarray_remove(ThreadDynarray_t ptr,
                           ThreadEntry_t const __entry);

/*
 * Erase all elements of the self.
 */
void ThreadDynarray_clear(ThreadDynarray_t ptr);

/*
 * Resets entry count to zero.
 */
void ThreadDynarray_reset(ThreadDynarray_t ptr);

/*
 * Moves count elements from src index to dst index.
 */
void ThreadDynarray_move(ThreadDynarray_t ptr, const unsigned long dst,
                         const unsigned long src, unsigned long count);

/* Compare the content of the entry pointed to by index with the content of
 * the entry addressed by __entry. The function returns true if the contents
 * are same.
 */
bool ThreadDynarray_compare(ThreadDynarray_t ptr,
                            const unsigned long index,
                            ThreadEntry_t const __entry);

/*
 * Returns a reference to a new ThreadDynarray new set is a clone of the self.
 */
ThreadDynarray_t ThreadDynarray_clone(ThreadDynarray_t ptr);

/*
 * Extends the capacity when the container is full.
 */
void ThreadDynarray_resize(ThreadDynarray_t ptr);

/*
 * Returns the number of elements.
 */
unsigned long ThreadDynarray_getCount(ThreadDynarray_t ptr);

/*
 * Returns the current storage capacity of the ThreadDynarray. This is guaranteed
 * to be at least as large as count().
 */
unsigned long ThreadDynarray_getCapacity(ThreadDynarray_t ptr);

/*
 * Returns upper bound of self (max index).
 */
unsigned long ThreadDynarray_getUpperBound(ThreadDynarray_t ptr);

/*
 * Returns lower bound of self (always zero).
 */
unsigned long ThreadDynarray_getLowerBound(ThreadDynarray_t ptr);

/*
 * Returns the size of the elements.
 */
unsigned long ThreadDynarray_getElementSize(ThreadDynarray_t ptr);

/*
 * Returns true if the size of self is zero.
 */
bool ThreadDynarray_isEmpty(ThreadDynarray_t ptr);

/*
 * Returns true if capacity available.
 */
bool ThreadDynarray(ThreadDynarray_t ptr);

/*
 * Returns true if the container is full.
 */
bool ThreadDynarray_is_full(ThreadDynarray_t ptr);

 /*
  * Returns true if capacity available.
  */
bool ThreadDynarray_getCapacityAvailable(ThreadDynarray_t ptr);

/* 
 * Assignement.
 */
ThreadDynarray_t ThreadDynarray_assign(ThreadDynarray_t src,
                                       ThreadDynarray_t dst);

/* 
 * Returns true if the dynamic arrays are equal.
 */
bool ThreadDynarray_areEquals(ThreadDynarray_t ptr1,
                              ThreadDynarray_t ptr2);

/* 
 * Returns true if the dynamic arrays are not equal.
 */
bool ThreadDynarray_areNotEquals(ThreadDynarray_t ptr1,
                                 ThreadDynarray_t ptr2);

void ThreadDynarray_lock(ThreadDynarray_t ptr);

void ThreadDynarray_unlock(ThreadDynarray_t ptr);


#endif                          /* #ifndef __THREAD_DYNARRAY_H__ */
