
#include <lstrings.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <allocator.h>

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
search(lstrings_t lstrings, const char* string)
{
	register link_t cur = lstrings->next;
	
	while(cur != ((link_t)&(lstrings->item)))
	{
		if(!strcmp((const char*)(cur->item),string))
			return cur;
			
		cur = cur->next;
	}
	
	errno = ESRCH;
	return NULL;
}



lstrings_t
lstrings_new(void)
{
	lstrings_t lstrings;
	
	if(!(lstrings = (lstrings_t)calloc(1,sizeof(s_lstrings_t))))
		return NULL;
		
	if(!allocator)
	{
		if(!(allocator = allocator_new(__DEFAULT_BLOCK_CAPACITY, sizeof(s_link_t), NULL)))
		{
			free(lstrings);
			return NULL;
		}
	}
	
	lstrings->next = lstrings->prev = lstrings->cur = (link_t)(&(lstrings->item));
	lstrings->pos = -1;
	lstrings->size = 0;
	
	ref++;
	return lstrings;
}

int
lstrings_rewind(lstrings_t lstrings)
{
	if(!lstrings)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!lstrings->size)
	{
		errno = EAGAIN;
		return 0;
	}
	
	lstrings->cur = lstrings->next;
	lstrings->pos = 0;
	
	return 1;
}

int
lstrings_unwind(lstrings_t lstrings)
{
	if(!lstrings)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!lstrings->size)
	{
		errno = EAGAIN;
		return 0;
	}
	
	lstrings->cur = lstrings->prev;
	lstrings->pos = lstrings->size - 1;
	
	return 1;
}

int
lstrings_clear(lstrings_t lstrings)
{
	if(!lstrings)
		return EINVAL;
	
	if(!lstrings->size)
		return 0;
	
	while(lstrings->size)
		lstrings_pop_back(lstrings);
	
	return 0;
}

int
lstrings_free(lstrings_t* lstrings_ptr)
{
	if(!(*lstrings_ptr))
		return EINVAL;
	
	if((*lstrings_ptr)->size)
		lstrings_clear(*lstrings_ptr);
	
	if(--ref)
		allocator_free(&allocator);
		
	free(*lstrings_ptr);
	*lstrings_ptr = NULL;
	
	return 0;
}

int
lstrings_push_front(lstrings_t lstrings, const char* string)
{
	link_t what, where, next;
     	
    if(!lstrings || !string)
    	return EINVAL;
    
    if(!(what = (link_t)link_new((void*)string)))
    	return errno;
    	
    where = (link_t)(&(lstrings->item));
    next = where->next;

    what->prev = where;
    what->next = next;
    next->prev = what;
    where->next = what;
    	
    lstrings->size++;
    
    /* the iteration functions are now not permited */
    lstrings->pos = -1;
    lstrings->cur = (link_t)(&(lstrings->item));	
    
    return 0;
}

int
lstrings_push_back(lstrings_t lstrings, const char* string)
{
	link_t what, where, prev;
	
	if(!lstrings || !string)
    	return EINVAL;
    
    if(!(what = (link_t)link_new((void*)string)))
    	return errno;
    	
    where = (link_t)(&(lstrings->item));
    prev = where->prev;
	what->next = where;
	what->prev = prev;
    prev->next = what;
    where->prev = what;
    
    lstrings->size++;
    
    /* the iteration functions are now not permited */
    lstrings->pos = -1;
    lstrings->cur = (link_t)(&(lstrings->item));
    
    return 0;	
}

int 
lstrings_insert_after(lstrings_t lstrings, const char* what, const char* where)
{
    link_t __what, __where, __next;
    
    if(!lstrings || !what || !where)
    	return EINVAL;
    
    if(!(__what = link_new((void*)what)))
    	return errno;
    	
    if((__where = search(lstrings, where)))
    	return errno;
    	
    __next = __where->next;

    __what->prev = __where;
    __what->next = __next;
    __next->prev = __what;
    __where->next = __what;
    
    lstrings->size++;
    
    return 0;
}

int 
lstrings_insert_before(lstrings_t lstrings, const char* what, const char* where)
{
    link_t __what, __where, __prev;
    
     if(!lstrings || !what || !where)
    	return EINVAL;
    
    if(!(__what = link_new((void*)what)))
    	return errno;
    	
    if((__where = search(lstrings, where)))
    	return errno;
    	
    __prev = __where->prev;
   	
   	__what->next = __where;
    __what->prev = __prev;
    __prev->next = __what;
    __where->prev = __what;
   
    lstrings->size++;
    
    return 0;
}

const char*
lstrings_pop_back(lstrings_t lstrings)
{
	link_t link, next, prev;
	const char* string;
	
	if(!lstrings)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!lstrings->size)
	{
		errno = EAGAIN;
		return NULL;
	}
	
	link = lstrings->prev;
	
	next = link->next;
    prev = link->prev;

    prev->next = next;
    next->prev = prev;
    
    lstrings->size--;

    string = (const char*)link->item;
    
    link_free((link_t*)&link);
    
    /* the iteration functions are now not permited */
    lstrings->pos = -1;
    lstrings->cur = (link_t)(&(lstrings->item));
    
    return string;
}

const char*
lstrings_pop_front(lstrings_t lstrings)
{
	
	link_t link, next, prev;
	const char* string;
	
	if(!lstrings)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!lstrings->size)
	{
		errno = EAGAIN;
		return NULL;
	}
	
	link = lstrings->next;
	
	next = link->next;
    prev = link->prev;
    prev->next = next;
    next->prev = prev;
    
    lstrings->size--;

    string = (const char*)link->item;
    
    link_free((link_t*)&link);
    
    /* the iteration functions are now not permited */
    lstrings->pos = -1;
    lstrings->cur = (link_t)(&(lstrings->item));

    return string;
}

int
lstrings_remove(lstrings_t lstrings, const char* string)
{
	link_t link, next, prev;
	
	if(!lstrings || !string)
		return EINVAL;
	
	if(!lstrings->size)
		return EAGAIN;
	
	if(!(link = search(lstrings, string)))
		return errno;
	
	next = link->next;
    prev = link->prev;
    prev->next = next;
    next->prev = prev;
    
    lstrings->size--;
    
    /* the iteration functions are now not permited */
    lstrings->pos = -1;
    lstrings->cur = (link_t)(&(lstrings->item));

    return link_free((link_t*)&link);
}

int
lstrings_get_size(lstrings_t lstrings)
{
	if(!lstrings)
	{
		errno = EINVAL;
		return -1;
	}
	
	return lstrings->size;
}

int
lstrings_contains(lstrings_t lstrings, const char* string)
{
	register link_t cur;
	
	if(!lstrings || !string)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!lstrings->size)
	{
		errno = EAGAIN;
		return 0;
	}
	
	cur = lstrings->next;
	
	while(cur != ((link_t)&(lstrings->item)))
	{
		if(!strcmp((const char*)(cur->item), string))
			return 1;
			
		cur = cur->next;
	}
	
	return 0;
}

int
lstrings_is_empty(lstrings_t lstrings)
{
	if(!lstrings)
	{
		errno = EINVAL;
		return 0;
	}
	
	return !lstrings->size;
}

int
lstrings_move_next(lstrings_t lstrings)
{
	if(!lstrings)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!lstrings->size || (-1 == lstrings->pos))
	{
		errno = EAGAIN;
		return 0;
	}
	
	if(lstrings->cur != (link_t)(&(lstrings->item)))
	{
		lstrings->cur = lstrings->cur->next;
		lstrings->pos++;
		return 1;
	}
	
	lstrings->pos = -1;
	errno = ERANGE;
	return 0;
}

const char*
lstrings_get(lstrings_t lstrings)
{
	if(!lstrings)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!lstrings->size || (-1 == lstrings->pos))
	{
		errno = EAGAIN;
		return NULL;
	}
	
	return (const char*)(lstrings->cur->item);
}

const char*
lstrings_set(lstrings_t lstrings, const char* string)
{
	const char* prev_string;
	
	if(!lstrings || !string)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!lstrings->size || (-1 == lstrings->pos))
	{
		errno = EAGAIN;
		return NULL;
	}
	
	prev_string = (const char*)(lstrings->cur->item);
	lstrings->cur->item = (void*)string;
	
	return prev_string;
}

const char*
lstrings_get_at(lstrings_t lstrings, int pos)
{
	register link_t cur;
	
	if(!lstrings || pos < 0 || pos >= lstrings->size)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!lstrings->size)
	{
		errno = EAGAIN;
		return NULL;
	}
	
	cur = lstrings->next;
	
	while(pos--)
		cur = cur->next;
		
	
	return (const char*)(cur->item);	
}

const char*
lstrings_set_at(lstrings_t lstrings, int pos, const char* string)
{
	register link_t cur;
	const char* prev_string;
	
	if(!lstrings || !string)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(pos < 0 || pos >= lstrings->size)
	{
		errno = ERANGE;
		return NULL;
	}
	
	if(!lstrings->size)
	{
		errno = EAGAIN;
		return NULL;
	}
	
	cur = lstrings->next;
	
	while(pos--)
		cur = cur->next;
		
	prev_string = (const char*)cur->item;	
	cur->item = (void*)string;
	
	return prev_string;
	
}

int
lstrings_move_prev(lstrings_t lstrings)
{
	if(!lstrings)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!lstrings->size || (-1 == lstrings->pos))
	{
		errno = EAGAIN;
		return 0;
	}
	
	if(lstrings->cur != (link_t)(&(lstrings->item)))
	{
		lstrings->cur = lstrings->cur->prev;
		lstrings->pos--;
		return 1;
	}
	
	lstrings->pos = -1;
	errno = ERANGE;
	return 0;
}

static int
seek_set(lstrings_t lstrings, int offset)
{
	if(offset > lstrings->size)
		return EINVAL;
	
	lstrings_rewind(lstrings);
	
	while(offset--)
		lstrings_move_next(lstrings);
		
	return 0;
}

static int
seek_end(lstrings_t lstrings, int offset)
{
	if(offset > lstrings->size)
		return EINVAL;
	
	lstrings_unwind(lstrings);
	
	while(offset--)
		lstrings_move_prev(lstrings);
		
	return 0;
}


static int
seek_cur(lstrings_t lstrings, int offset)
{
	if(lstrings->cur == lstrings->next)
	{
		/* we are at the begin of the lstrings */
		seek_set(lstrings, offset);
	}
	else if(lstrings->cur == lstrings->prev)
	{
		/* we are at the end of the lstrings */
		seek_end(lstrings, offset);
	}
	else
	{
		if(offset > (lstrings->size - lstrings->pos + 1))	
			return EINVAL;
			
		while(offset--)
			lstrings_move_next(lstrings);
			
	}
		
	return 0;
}

int
lstrings_seek(lstrings_t lstrings, int offset, int whence)
{
	if(!lstrings)
		return EINVAL;
		
	if(!lstrings->size)
		return EAGAIN;
	
	switch(whence)
	{
		case SEEK_SET :
		return seek_set(lstrings, offset);
		
		case SEEK_CUR :
		return seek_cur(lstrings, offset);
		
		case SEEK_END :
		return seek_end(lstrings, offset);
		
	}
	
	return EINVAL;
}

int
lstrings_tell(lstrings_t lstrings)
{
	if(!lstrings)
	{
		errno = EINVAL;
		return -1;
	}
	
	if(!lstrings->size || (-1 == lstrings->pos))
	{
		errno = EAGAIN;
		return -1;
	}
	
	return lstrings->pos;
}

int
lstrings_getpos(lstrings_t lstrings, int* pos)
{
	if(!lstrings || !pos)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(!lstrings->size || (-1 == lstrings->pos))
	{
		errno = EAGAIN;
		return 0;
	}
	
	*pos = lstrings->pos;
	return 1;
}

int
lstrings_setpos(lstrings_t lstrings, int pos)
{
	if(!lstrings)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(pos < 0 || pos >= lstrings->size)
	{
		errno = ERANGE;
		return 0;
	}
	
	if(!lstrings->size)
	{
		errno = EAGAIN;
		return 0;
	}
	
	lstrings->cur = lstrings->next;
	lstrings->pos = pos;
	
	while(pos--)
		lstrings->cur = lstrings->cur->next;
		
	return 1;
}


const char*
lstrings_get_front(lstrings_t lstrings)
{
	if(!lstrings)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!lstrings->size)
	{
		errno = EAGAIN;
		return NULL;
	}
	
	return (const char*)(lstrings->next->item);	
}


const char*
lstrings_get_back(lstrings_t lstrings)
{
	if(!lstrings)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!lstrings->size)
	{
		errno = EAGAIN;
		return NULL;
	}
	
	return (const char*)(lstrings->prev->item);
}

char**
lstrings_to_cstr(lstrings_t lstrings)
{
	register int i;
	link_t cur;
	int size;
	char** cstr;
	
	if(!lstrings || !lstrings->size)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!(cstr = (char**)calloc(lstrings->size, sizeof(char*))))
		return NULL;
	
	/* get the first link of the list */
	cur = lstrings->next;	
	
	for(i = 0; i < size; i++)
	{
		if(!(cstr[i] = strdup(cur->item)))
		{
			register int j;
			
			for(j = 0; j <i; j++)
				free(cstr[j]);
				
			free(cstr);
			
			return NULL;
		}
		
		cur = cur->next;
	}
	
	
	return cstr;
}
