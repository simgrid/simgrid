
#include <list.h>
#include <errno.h>
#include <stdlib.h>

#define __DEFAULT_BLOCK_CAPACITY 128

static allocator_t
allocator = NULL;

static int
ref = 0;

static link_t
link_new(void* item)
{
	link_t link;
	
	if(!item)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!(link = (link_t)allocator_alloc(allocator)))
		return NULL;
	
	link->next = link->prev = NULL;
	link->item = item;
	
	return link;
}

static int
link_free(link_t* ref)
{
	if(!(*ref))
		return  EINVAL;
		
	allocator_dealloc(allocator, *ref);
	*ref = NULL;
	
	return 0;
}

static link_t
search(list_t list, void* item)
{
	register link_t cur = list->next;
	
	while(cur != ((link_t)&(list->item)))
	{
		if(cur->item == item)
			return cur;
			
		cur = cur->next;
	}
	
	errno = ESRCH;
	return NULL;
}



list_t
list_new(fn_finalize_t fn_finalize)
{
	list_t list = (list_t)calloc(1,sizeof(s_list_t));
	
	if(!list)
		return NULL;
		
	if(!allocator)
	{
		if(!(allocator = allocator_new(__DEFAULT_BLOCK_CAPACITY, sizeof(s_link_t), NULL)))
		{
			free(list);
			return NULL;
		}
	}
			
	list->next = list->prev = list->cur = (link_t)(&(list->item));
	list->fn_finalize = fn_finalize;
	list->pos = -1;
	list->size = 0;
	
	ref++;
	
	return list;
}

int
list_rewind(list_t list)
{
	if(!list)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!list->size)
	{
		errno = EAGAIN;
		return 0;
	}
	
	list->cur = list->next;
	list->pos = 0;
	
	return 1;
}

int
list_unwind(list_t list)
{
	if(!list)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!list->size)
	{
		errno = EAGAIN;
		return 0;
	}
	
	list->cur = list->prev;
	list->pos = list->size - 1;
	
	return 1;
}

int
list_clear(list_t list)
{
	int rv;
	
	if(!list)
		return EINVAL;
	
	if(!list->size)
		return EAGAIN;
	
	if(list->fn_finalize)
	{
		void* item;
		
		while((item = list_pop_back(list)))
			if(0 != (rv = (*(list->fn_finalize))(&item)))
				return rv;
	}
	else
		while(list_pop_back(list));
	
	return 0;
}

int
list_free(list_t* list_ptr)
{
	if(!(*list_ptr))
		return EINVAL;
	
	if((*list_ptr)->size)
		list_clear(*list_ptr);
		
	if(--ref)
		allocator_free(&allocator);
	
	free(*list_ptr);
	*list_ptr = NULL;
	
	return 0;
}

int
list_push_front(list_t list, void* item)
{
	link_t what, where, next;
     	
    if(!list || !item)
    	return EINVAL;
    
    if(!(what = (link_t)link_new(item)))
    	return errno;
    	
    where = (link_t)(&(list->item));
    next = where->next;

    what->prev = where;
    what->next = next;
    next->prev = what;
    where->next = what;
    	
    list->size++;
    
    /* the iteration functions are now not permited */
    list->pos = -1;
    list->cur = (link_t)(&(list->item));
    
    return 0;
}

int
list_push_back(list_t list, void* item)
{
	link_t what, where, prev;
	
	if(!list || !item)
    	return EINVAL;
    
    if(!(what = (link_t)link_new(item)))
    	return errno;
    	
    where = (link_t)(&(list->item));
    prev = where->prev;
	what->next = where;
	what->prev = prev;
    prev->next = what;
    where->prev = what;
  
    list->size++;
    
    /* the iteration functions are now not permited */
    list->pos = -1;
    list->cur = (link_t)(&(list->item));
    
    return 0;	
}

int 
list_insert_after(list_t list, void* what, void* where)
{
    link_t __what, __where, __next;
    
    if(!list || !what || !where)
    	return EINVAL;
    
    if(!(__what = link_new(what)))
    	return errno;
    	
    if((__where = search(list, where)))
    	return errno;
    	
    __next = __where->next;

    __what->prev = __where;
    __what->next = __next;
    __next->prev = __what;
    __where->next = __what;
    
    list->size++;
    
    return 0;
}

int 
list_insert_before(list_t list, void* what, void* where)
{
    link_t __what, __where, __prev;
    
     if(!list || !what || !where)
    	return EINVAL;
    
    if(!(__what = link_new(what)))
    	return errno;
    	
    if((__where = search(list, where)))
    	return errno;
    	
    __prev = __where->prev;
   	
   	__what->next = __where;
    __what->prev = __prev;
    __prev->next = __what;
    __where->prev = __what;
   
    list->size++;
    
    return 0;
}


void*
list_pop_back(list_t list)
{
	link_t link, next, prev;
	void* item;
	
	if(!list)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!list->size)
	{
		errno = EAGAIN;
		return NULL;
	}
	
	link = list->prev;
	
	next = link->next;
    prev = link->prev;
    prev->next = next;
    next->prev = prev;
    
    list->size--;
    
    item = link->item;
    
    link_free((link_t*)&link);
    
    /* the iteration functions are now not permited */
    list->pos = -1;
    list->cur = (link_t)(&(list->item));
    
    return item;
}

void*
list_pop_front(list_t list)
{
	
	link_t link, next, prev;
	void* item;
	
	if(!list)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!list->size)
	{
		errno = EAGAIN;
		return NULL;
	}
	
	link = list->next;
	
	next = link->next;
    prev = link->prev;
    prev->next = next;
    next->prev = prev;
    
    list->size--;

    item = link->item;
    link_free((link_t*)&link);
    
    /* the iteration functions are now not permited */
    list->pos = -1;
    list->cur = (link_t)(&(list->item));

    return item;
}

int
list_remove(list_t list, void* item)
{
	link_t link, next, prev;
	
	if(!list || !item)
		return EINVAL;
	
	if(!list->size)
		return EAGAIN;
	
	if(!(link = search(list, item)))
		return errno;
	
	next = link->next;
    prev = link->prev;
    prev->next = next;
    next->prev = prev;	
    
    list->size--;
    
    /* the iteration functions are now not permited */
    list->pos = -1;
    list->cur = (link_t)(&(list->item));

    return link_free((link_t*)&link);
}

int
list_get_size(list_t list)
{
	if(!list)
	{
		errno = EINVAL;
		return -1;
	}
	
	return list->size;
}

int
list_contains(list_t list, void* item)
{
	register link_t cur;
	
	if(!list || !item)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!list->size)
	{
		errno = EAGAIN;
		return 0;
	}
	
	cur = list->next;
	
	while(cur != ((link_t)&(list->item)))
	{
		if(cur->item == item)
			return 1;
			
		cur = cur->next;
	}
	
	return 0;
}

int
list_is_empty(list_t list)
{
	if(!list)
	{
		errno = EINVAL;
		return 0;
	}
	
	return !list->size;
}

int
list_is_autodelete(list_t list)
{
	if(!list)
	{
		errno = EINVAL;
		return 0;
	}
	
	return NULL != list->fn_finalize;
}

int
list_move_next(list_t list)
{
	if(!list)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!list->size || (-1 == list->pos))
	{
		errno = EAGAIN;
		return 0;
	}
	
	if(list->cur != (link_t)(&(list->item)))
	{
		list->cur = list->cur->next;
		list->pos++;
		return 1;
	}
	
	list->pos = -1;
	errno = ERANGE;
	return 0;
}

void*
list_get(list_t list)
{
	if(!list)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!list->size || (-1 == list->pos))
	{
		errno = EAGAIN;
		return NULL;
	}
	
	return list->cur->item;	
}

void*
list_set(list_t list, void* item)
{
	void* prev_item;
	
	if(!list || !item)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!list->size  || (-1 == list->pos))
	{
		errno = EAGAIN;
		return NULL;
	}
	
	prev_item = list->cur->item;
	list->cur->item = item;
	
	return prev_item;
}

void*
list_get_at(list_t list, int pos)
{
	register link_t cur;
	
	if(!list)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(pos < 0 || pos >= list->size)
	{
		errno = ERANGE;
		return NULL;
	}
	
	if(!list->size)
	{
		errno = EAGAIN;
		return NULL;
	}
	
	cur = list->next;
	
	while(pos--)
		cur = cur->next;
		
	
	return cur->item;	
}

void*
list_set_at(list_t list, int pos, void* item)
{
	register link_t cur;
	void* prev_item;
	
	if(!list || !item)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(pos < 0 || pos >= list->size)
	{
		errno = ERANGE;
		return NULL;
	}
	
	if(!list->size)
	{
		errno = EAGAIN;
		return NULL;
	}
	
	cur = list->next;
	
	while(pos--)
		cur = cur->next;
		
	prev_item = cur->item;	
	cur->item = item;
	
	return prev_item;
	
}

int
list_move_prev(list_t list)
{
	if(!list)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!list->size  || (-1 == list->pos))
	{
		errno = EAGAIN;
		return 0;
	}
	
	if(list->cur != (link_t)(&(list->item)))
	{
		list->cur = list->cur->prev;
		list->pos--;
		return 1;
	}
	
	list->pos = -1;
	errno = ERANGE;
	return 0;
}

static int
seek_set(list_t list, int offset)
{
	if(offset > list->size)
		return EINVAL;
	
	list_rewind(list);
	
	while(offset--)
		list_move_next(list);
		
	return 0;
}

static int
seek_end(list_t list, int offset)
{
	if(offset > list->size)
		return EINVAL;
	
	list_unwind(list);
	
	while(offset--)
		list_move_prev(list);
		
	return 0;
}


static int
seek_cur(list_t list, int offset)
{
	if(list->cur == list->next)
	{
		/* we are at the begin of the list */
		seek_set(list, offset);
	}
	else if(list->cur == list->prev)
	{
		/* we are at the end of the list */
		seek_end(list, offset);
	}
	else
	{
		if(offset > (list->size - list->pos + 1))	
			return EINVAL;
			
		while(offset--)
			list_move_next(list);
			
	}
		
	return 0;
}

int
list_seek(list_t list, int offset, int whence)
{
	if(!list)
		return EINVAL;
		
	if(!list->size)
		return EAGAIN;
	
	switch(whence)
	{
		case SEEK_SET :
		return seek_set(list, offset);
		
		case SEEK_CUR :
		return seek_cur(list, offset);
		
		case SEEK_END :
		return seek_end(list, offset);
		
	}
	
	return EINVAL;
}

int
list_tell(list_t list)
{
	if(!list)
	{
		errno = EINVAL;
		return -1;
	}
	
	if(!list->size || (-1 == list->pos))
	{
		errno = EAGAIN;
		return -1;
	}
	
	return list->pos;
}

int
list_getpos(list_t list, int* pos)
{
	
	if(!list || !pos)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!list->size || (-1 == list->pos))
	{
		errno = EAGAIN;
		return 0;
	}
	
	*pos = list->pos;
	return 1;
}

int
list_setpos(list_t list, int pos)
{
	if(!list)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(pos < 0 || pos >= list->size)
	{
		errno = ERANGE;
		return 0;
	}
	
	if(!list->size)
	{
		errno = EAGAIN;
		return 0;
	}
	
	list->cur = list->next;
	list->pos = pos;
	
	while(pos--)
		list->cur = list->cur->next;
		
	return 1;
}


void*
list_get_front(list_t list)
{
	if(!list)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!list->size)
	{
		errno = EAGAIN;
		return NULL;
	}
	
	return list->next->item;	
}


void*
list_get_back(list_t list)
{
	if(!list)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!list->size)
	{
		errno = EAGAIN;
		return NULL;
	}
	
	return list->prev->item;
}
