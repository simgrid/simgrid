/** 
 * File : private/vector.h
 *
 * Copyright 2006,2007 Malek Cherier, Martin Quinson. All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it under the terms 
 * of the license (GNU LGPL) which comes with this package. 
 */

#ifndef __VECTOR_H
#define __VECTOR_H

#ifdef __cplusplus
extern "C" {
#endif


#ifndef __FN_FINALIZE_T_DEFINED
typedef int (*fn_finalize_t)(void**);
#define __FN_FINALIZE_T_DEFINED
#endif

#ifndef SEEK_SET
#define SEEK_SET				0
#endif 

#ifndef SEEK_CUR
#define SEEK_CUR				1
#endif

#ifndef SEEK_END
#define SEEK_END				2
#endif

/*
 * this type represents a vector of void* pointers.
 */
typedef struct s_vector
{
	int size;                		/* the number of items of the vector                                    */						
	int capacity;            		/* the capacity of the vector                                           */			
	void** items;	                	/* the items of the vector                                          */								
	fn_finalize_t fn_finalize;    	/* a pointer to a function used to cleanup the elements of the vector   */
	int pos;					
}s_vector_t,* vector_t;

vector_t
vector_new(int capacity,fn_finalize_t fn_finalize);

int 
vector_clear(vector_t vector);

int
vector_free(vector_t* vector_ptr);

int
vector_is_empty(vector_t vector);

void*
vector_get_at(vector_t vector, int pos);

int 
vector_get_size(vector_t vector);

void*
vector_get_front(vector_t vector);

void*
vector_get_back(vector_t vector);

int
vector_get_capacity_available(vector_t vector);

int
vector_push_back(vector_t vector, void* item);

void*
vector_pop_back(vector_t vector);

int
vector_get_upper_bound(vector_t vector);

void*
vector_set_at(vector_t vector, int index, void* item);

int 
vector_insert(vector_t vector, int index, void* item);

int
vector_erase_at(vector_t vector, int index);

int 
vector_erase(vector_t vector, void* item);

int
vector_erase_range(vector_t vector, int first, int last);

int 
vector_remove(vector_t vector, void* item);

int
vector_search(vector_t vector, void* item);

int
vector_assign(vector_t dst,vector_t src);

int
vector_get_capacity(vector_t vector);

int
vector_equals(vector_t vector, vector_t other);

int
vector_swap(vector_t vector, vector_t other);

vector_t
vector_clone(vector_t vector);

int
vector_contains(vector_t vector,void* item);

int
vector_reserve(vector_t vector,int size);

int
vector_is_autodelete(vector_t vector);

int
vector_has_capacity_available(vector_t vector);

int
vector_is_full(vector_t vector);

int
vector_get_max_index(vector_t vector);

void*
vector_get(vector_t vector);

void*
vector_get_at(vector_t vector, int pos);

int
vector_getpos(vector_t vector, int* pos);

int
vector_move_next(vector_t vector);

int
vector_move_prev(vector_t vector);

int
vector_rewind(vector_t vector);

int
vector_seek(vector_t vector, int offset, int whence);

void*
vector_set(vector_t vector, void* item);

int
vector_setpos(vector_t vector, int pos);

int
vector_tell(vector_t vector);

int
vector_unwind(vector_t vector);


#ifdef __cplusplus
}
#endif

#endif /* !XBT_PRIVATE_VECTOR_PTR_H */
