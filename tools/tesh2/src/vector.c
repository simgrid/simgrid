
#include <vector.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define inline _inline
#endif

static inline int 
resize(vector_t vector)
{	    
	return vector_reserve(vector, !(vector->capacity) ? 1 : (vector->size) << 1);
}

static inline int
move(vector_t vector, int dst,int src, int size)
{
	return (NULL != memmove(vector->items + dst, vector->items + src,size * sizeof(void*)));
}

vector_t
vector_new(int capacity, fn_finalize_t fn_finalize)
{
	vector_t vector;
	
	if(capacity < 0)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!(vector = (vector_t)calloc(1,sizeof(s_vector_t))))
		return NULL;
	
	if(capacity)
	{
		if(!(vector->items = (void**)calloc(capacity,sizeof(void*))))
		{
			free(vector);
			return NULL;
		}
		
		vector->capacity = capacity;
	}
				
	vector->fn_finalize = fn_finalize;
	vector->pos = -1;
	return vector;
}

int 
vector_clear(vector_t vector)
{
	if(!vector)
	    return EINVAL;
	    
	if(!vector->size)
		return EAGAIN;
	
	if(vector->fn_finalize)
	{
		int size = vector->size;
		fn_finalize_t fn_finalize = vector->fn_finalize;
		void** items = vector->items;
		register int pos;
		
		for(pos = 0; pos < size; pos++)
		{
			if((errno = (*(fn_finalize))(&(items[pos]))))
				return errno;
			else
				vector->size--;
		}
	}
	else
	{
		vector->size = 0;	
	}
	
	vector->pos = -1;
	
	return 0;
}

int 
vector_free(vector_t* vector_ptr)
{
    if(!(*vector_ptr))
        return EINVAL;
       
	if((errno = vector_clear(*vector_ptr)))
		return errno;

	free(*vector_ptr);
	*vector_ptr = NULL;
	
	return 0;
}

int 
vector_is_empty(vector_t vector)
{	
	if(!vector)
	{
	    errno = EINVAL;
	    return 0;
	}
	
	return !vector->size;
}

int 
vector_get_size(vector_t vector)
{
    if(!vector)
    {
        errno = EINVAL;
        return -1;
    }
    
	return vector->size;
}

void* 
vector_get_front(vector_t vector)
{
	if(!vector)
    {
        errno = EINVAL;
        return NULL;
    }
    
    if(!vector->size)
    {
    	errno = EAGAIN;
    	return NULL;
    }
    
	return vector->items[0];
}

void*
vector_get_back(vector_t vector)
{
	if(!vector)
    {
        errno = EINVAL;
        return NULL;
    }
    
    if(!vector->size)
    {
    	errno = EAGAIN;
    	return NULL;
    }
    
	return vector->items[vector->size - 1];
}

int
vector_get_capacity_available(vector_t vector)
{
    if(!vector)
    {
        errno = EINVAL;
        return -1;
    }
    
	return (vector->capacity - vector->size);
}

int
vector_push_back(vector_t vector, void* item)
{
	if(!vector || !item)
    	return EINVAL;
    
    
	/* if all capacity is used, resize the vector */
	if(vector->capacity <= vector->size)
	{
		if(!resize(vector))
			return errno;
	}
	
	/* increment the item count and push the new item at the end of the vector */
	vector->items[++(vector->size) - 1] = item;
	
	vector->pos = -1;
	
	return 0;	
}

void* 
vector_pop_back(vector_t vector)
{
    if(!vector)
    {
        errno = EINVAL;
        return NULL;
    }
    
    if(!vector->size)
    {
    	errno = EAGAIN;
    	return NULL;
    }
   	
   	vector->pos = -1;
   	
	return vector->items[(vector->size)-- - 1];
}

int
vector_get_upper_bound(vector_t vector)
{
	if(!vector)
    {
        errno = EINVAL;
        return -1;
    }
    
    if(!vector->size)
    {
    	errno = EAGAIN;
    	return -1;
    }
    
	return (vector->size - 1);
}

void* 
vector_set_at(vector_t vector, int pos, void* item)
{
	void* prev_item;

	if(!vector)
    {
        errno = EINVAL;
        return NULL;
    }
   
   	if(!vector->size)
   	{
   		errno = EAGAIN;
   		return NULL;
   	}
   	
   	if((pos < 0) || (pos >= vector->size))
   	{
   		errno = ERANGE;
   		return NULL;
   	}
	
	prev_item = vector->items[pos];
	vector->items[pos] = item;
	return prev_item;
}

int 
vector_insert(vector_t vector, int pos, void* item)
{
	if(!vector)
        return EINVAL;
   
   	if(!vector->size)
   	{
   		errno = EAGAIN;
   		return 0;
   	}
   	
   	if((pos < 0) || (pos >= vector->size))
   	{
   		errno = ERANGE;
   		return 0;
   	}
    
	if(vector->size >= vector->capacity)
	{
		if(!resize(vector))
			return errno;
	}		
	
	if(vector->size)
	{
		if(!move(vector, pos + 1, pos, vector->size - pos))
		    return errno;
    }
	
	vector->size++;
	vector->items[pos] = item;
	vector->pos = -1;
	
	return 0;
}

int 
vector_erase_at(vector_t vector, int pos)
{
	if(!vector)
        return EINVAL;
    
    if(!vector->size)
   	{
   		errno = EAGAIN;
   		return 0;
   	}
   	
   	if((pos < 0) || (pos >= vector->size))
   	{
   		errno = ERANGE;
   		return 0;
   	}

	if(vector->fn_finalize)
	{
		if((errno = (*(vector->fn_finalize))(&(vector->items[pos]))))
			return errno;
	}

	if(pos != (vector->size - 1))
	{
		if(!move(vector, pos, pos + 1, (vector->size - (pos + 1))))
		    return errno;
	}
	
	vector->size--;
	vector->pos = -1;
	
	return 0;
}

int 
vector_erase(vector_t vector, void* item)
{
	int pos;
	
	if(!vector || !item)
		return EINVAL;
	
	if(!vector->size)
   		return EAGAIN;
   		
	if(-1 == (pos = vector_search(vector,item)))
		return errno;
		
	if(vector->fn_finalize)
	{
		if((errno = (*(vector->fn_finalize))(&item)))
			return errno;	
	}
	
	if(pos != (vector->size - 1))
		if(!move(vector, pos, pos + 1, (vector->size - (pos + 1))))
			return errno;
	
	vector->size--;
	
	vector->pos = -1;
	
	return 0;
}

int 
vector_erase_range(vector_t vector, int first, int last)
{
	register int width;

	if(!vector || first >= last)
		return EINVAL;
	
	if(vector->size < 2)
		return EAGAIN;
		
	if((first < 0) || (last < 0) || (last >= vector->size))
		return ERANGE;
	
	width = (last - first) + 1;

	while(width--)
	{
		if((errno = vector_erase_at(vector,first)))
			return errno;
	}
		
	return 0;
}

int
vector_search(vector_t vector, void* item)
{
	register int pos;
	void** items;
	int size;

	if(!vector || !item)
	{
		errno = EINVAL;
		return -1;
	}
	
	if(!vector->size)
   	{
   		errno = EAGAIN;
   		return -1;
   	}
	
	vector = (vector_t)vector;
	
	items = vector->items;
	size = vector->size;
	
	for(pos = 0 ; pos < size; pos++)
	{
		if(items[pos] == item)
			return pos;
	}
	
	errno = ESRCH;
	return -1;  	
}

int 
vector_remove(vector_t vector, void* item)
{
	int pos;
	
	if(!vector || !item)
		return EINVAL;
	
	if(!vector->size)
   		return EAGAIN;
   		
	if(-1 == (pos = vector_search(vector,item)))
		return errno;
	
	if(pos != (vector->size - 1))
		if(!move(vector, pos, pos + 1, (vector->size - (pos + 1))))
			return errno;
	
	vector->size--;
	
	vector->pos = -1;
	
	return 0;
}

int
vector_assign(vector_t dst,vector_t src)
{
	register int pos;
	int size;
	void** items;

	
	if(!dst || !src ||(dst == src))
		return EINVAL;
	
	if(!src->size)
   		return EAGAIN;
	
	size = src->size;
	
	/* if the destination vector has not enough capacity resize it */
	if(size > dst->capacity)
	{
		if((errno = vector_reserve(dst, size - dst->capacity)))
			return errno;
	}

	/* clear the destination vector */
	if((errno = vector_clear(dst)))
		return errno;

	dst->fn_finalize = NULL;
		
	items = src->items;
	
	/* file the destination vector */
	for(pos = 0; pos < size; pos++)
		 if((errno = vector_push_back(dst,items[pos])))
		 	return errno;
		 	
	dst->pos = -1;
		 
	return 0;
}

int
vector_get_capacity(vector_t vector)
{
	if(!vector)
	{
		errno = EINVAL;
		return -1;
	}
		
	return vector->capacity;
}

int
vector_equals(vector_t first, vector_t second)
{
	register int pos;
	int size;
	void** __first, ** __second;

	
	if(!first || !second || (first == second))
	{
		errno = EINVAL;
		return 0;
	}
	
	if(first->size != second->size)
		return 0;
	
	size = first->size;
	__first = first->items;
	__second = second->items;
	
	for(pos = 0; pos < size; pos++)
	{
		if(__first[pos] != __second[pos])
			return 0;
	}
	
	return 1;
}

vector_t 
vector_clone(vector_t vector)
{
	int size;
	register int pos;
	void** dst,** src;
	vector_t clone;
	
	if(!vector)
	{
		errno = EINVAL;
		return NULL;
	}
	
	clone = vector_new(vector->capacity,NULL);

	if(!clone)
	    return NULL;
	
	size = vector->size;
	src = vector->items;
	dst = clone->items;
	
	for(pos = 0; pos < size; pos++)
		dst[pos] = src[pos];
	
	clone->size = size;
		
	return clone;
}

int
vector_contains(vector_t vector,void* item)
{
	if(!vector || !item)
	{
		errno = EINVAL;
		return 0;
	}
	
	return (-1 != vector_search(vector,item));
}

int
vector_swap(vector_t first,vector_t second)
{
	s_vector_t tmp;
	
	if(!first || !second)
		return EINVAL;
	
	/* save the content or first */
	tmp = *first;
	
	/* copy second in first */
	if(!memcpy(first, second, sizeof(s_vector_t)))
	    return 0;
	
	/* copy tmp in first */
	if(!memcpy(second, &tmp, sizeof(s_vector_t)))
	{
		*first = tmp;
	    return 0;
	}

	return 1;
}

int
vector_reserve(vector_t vector, int size)
{
	void** items;

	if(!vector || (size < 0))
		return EINVAL;
		
	if(vector->capacity >= size)
		return EAGAIN;
	
	if(!(items = (void**)realloc(vector->items, size * sizeof(void*))))
		return errno;
	
	vector->capacity = size;
	vector->items = items;
	
	return 0;
}

int
vector_is_autodelete(vector_t vector)
{
	if(!vector)
	{
		errno = EINVAL;
		return 0;
	}

	return NULL != vector->fn_finalize;
}

int
vector_has_capacity_available(vector_t vector)
{
	if(!vector)
	{
		errno = EINVAL;
		return 0;
	}
	
	return (vector->capacity > vector->size);
}

int
vector_is_full(vector_t vector)
{
	if(!vector)
    {
        errno = EINVAL;
        return 0;
    }
    

	return (vector->capacity == vector->size);
}

int
vector_get_max_index(vector_t vector)
{
	
	if(!vector)
	{
		errno = EINVAL;
		return -1;
	}
	
	
	return vector->size - 1;
}

void*
vector_get(vector_t vector)
{
	if(!vector)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!vector->size || (-1 == vector->pos))
	{
		errno = EAGAIN;
		return NULL;
	}
	return vector->items[vector->pos];	
}

void*
vector_get_at(vector_t vector, int pos)
{
	if(!vector)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(pos < 0 || pos >= vector->size)
	{
		errno = ERANGE;
		return NULL;
	}
	
	if(!vector->size)
	{
		errno = EAGAIN;
		return NULL;
	}
		
	
	return vector->items[pos];	
}

int
vector_getpos(vector_t vector, int* pos)
{
	
	if(!vector || !pos)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!vector->size || (-1 == vector->pos))
	{
		errno = EAGAIN;
		return 0;
	}
	
	*pos = vector->pos;
	return 1;
}

int
vector_move_next(vector_t vector)
{
	if(!vector)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!vector->size || (-1 == vector->pos))
	{
		errno = EAGAIN;
		return 0;
	}
	
	if(vector->pos < (vector->size - 1))
	{
		vector->pos++;
		return 1;
	}
	
	vector->pos = -1;
	errno = ERANGE;
	return 0;
}

int
vector_move_prev(vector_t vector)
{
	if(!vector)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!vector->size  || (-1 == vector->pos))
	{
		errno = EAGAIN;
		return 0;
	}
	
	if(vector->pos > 0)
	{
		vector->pos--;
		return 1;
	}
	
	vector->pos = -1;
	errno = ERANGE;
	return 0;
	
}

int
vector_rewind(vector_t vector)
{
	if(!vector)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!vector->size)
	{
		errno = EAGAIN;
		return 0;
	}
	
	vector->pos = 0;
	
	return 1;
}

static int
seek_set(vector_t vector, int offset)
{
	if(offset > vector->size)
		return EINVAL;
	
	vector_rewind(vector);
	
	while(offset--)
		vector_move_next(vector);
		
	return 0;
}

static int
seek_end(vector_t vector, int offset)
{
	if(offset > vector->size)
		return EINVAL;
	
	vector_unwind(vector);
	
	while(offset--)
		vector_move_prev(vector);
		
	return 0;
}


static int
seek_cur(vector_t vector, int offset)
{
	if(vector->pos == 0)
	{
		/* we are at the begin of the vector */
		seek_set(vector, offset);
	}
	else if(vector->pos == vector->size - 1)
	{
		/* we are at the end of the vector */
		seek_end(vector, offset);
	}
	else
	{
		if(offset > (vector->size - vector->pos + 1))	
			return EINVAL;
			
		while(offset--)
			vector_move_next(vector);
			
	}
		
	return 0;
}

int
vector_seek(vector_t vector, int offset, int whence)
{
	if(!vector)
		return EINVAL;
		
	if(!vector->size)
		return EAGAIN;
	
	switch(whence)
	{
		case SEEK_SET :
		return seek_set(vector, offset);
		
		case SEEK_CUR :
		return seek_cur(vector, offset);
		
		case SEEK_END :
		return seek_end(vector, offset);
		
	}
	
	return EINVAL;
}

void*
vector_set(vector_t vector, void* item)
{
	void* prev_item;
	
	if(!vector || !item)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!vector->size  || (-1 == vector->pos))
	{
		errno = EAGAIN;
		return NULL;
	}
	
	prev_item = vector->items[vector->pos];
	vector->items[vector->pos] = item;
	
	return prev_item;
}

int
vector_setpos(vector_t vector, int pos)
{
	if(!vector)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(pos < 0 || pos >= vector->size)
	{
		errno = ERANGE;
		return 0;
	}
	
	if(!vector->size)
	{
		errno = EAGAIN;
		return 0;
	}
	
	vector->pos = pos;
	return 1;
}

int
vector_tell(vector_t vector)
{
	if(!vector)
	{
		errno = EINVAL;
		return -1;
	}
	
	if(!vector->size || (-1 == vector->pos))
	{
		errno = EAGAIN;
		return -1;
	}
	
	return vector->pos;
}

int
vector_unwind(vector_t vector)
{
	if(!vector)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!vector->size)
	{
		errno = EAGAIN;
		return 0;
	}
	
	vector->pos = vector->size - 1;
	return 1;
}
