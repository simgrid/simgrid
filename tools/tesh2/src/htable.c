
#include <htable.h>

#include <errno.h>
#include <stdlib.h>	/* calloc()	free() */

#define __DEFAULT_BLOCK_CAPACITY	((int)512)
#define __DEFAULT_TABLE_SIZE		((int)256)


static hassoc_t
get_assoc(htable_t htable, const void* key, unsigned int* hval)
{
	register hassoc_t hassoc;
	hassoc_t* content = htable->content;
	fn_cmp_key_t fn_cmp_key = htable->fn_cmp_key;
	
	*hval = (*(htable->fn_hfunc))(key) % htable->size;

	for (hassoc = content[*hval]; hassoc; hassoc = hassoc->next)
		if((*fn_cmp_key)(hassoc->key,key))
			return hassoc;
	
	return NULL;
}


htable_t
htable_new(
				int block_capacity, 
				int size, 
				fn_hfunc_t fn_hfunc, 
				fn_cmp_key_t fn_cmp_key, 
				fn_finalize_t fn_finalize
)
{
	htable_t htable;
	
	if((block_capacity < 0) || (size < 0) ||!fn_hfunc || !fn_cmp_key)
	{
		errno = EINVAL;
		return NULL;
	}
	
		
	if(!(htable = (htable_t)calloc(1, sizeof(s_htable_t))))
		return NULL;
	
	if(!(htable->content = (hassoc_t*)calloc(size ? size : __DEFAULT_TABLE_SIZE, sizeof(hassoc_t))))
	{
		free(htable);
		return NULL;
	}
	
	if(!(htable->allocator = allocator_new(block_capacity ? block_capacity : __DEFAULT_BLOCK_CAPACITY, sizeof(s_hassoc_t),NULL)))
	{
		free(htable->content);
		free(htable);
		return NULL;
	}
	
	htable->size = size;
	htable->fn_hfunc = fn_hfunc;
	htable->fn_cmp_key = fn_cmp_key;
	htable->fn_finalize = fn_finalize;
	
	return htable;
}

int
htable_set(htable_t htable, const void* key, const void* val)
{
	hassoc_t hassoc;
	unsigned int hval;
	
	if(!htable || !key || !val)
		return EINVAL;
	
	
	if(!(hassoc = get_assoc(htable, key, &hval)))
	{
		if(!(hassoc = (hassoc_t)allocator_alloc(htable->allocator)))
			return errno;
		
		hassoc->key = key;
		hassoc->val = val;
		
		hassoc->next = (htable->content)[hval];
		(htable->content)[hval] = hassoc;
		
	}
	else
		hassoc->val = val; 
	
	return 0;  
}

void*
htable_lookup(htable_t htable, const void* key)
{
	hassoc_t hassoc;
	unsigned int hval;
	
	if(!htable || !key)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!(hassoc = get_assoc(htable, key, &hval)))
	{
		errno = ESRCH;
		return NULL;
	}
	
	return (void*)(hassoc->val);
	
}

void*
htable_remove(htable_t htable, const void* key)
{
	register hassoc_t hassoc;
	hassoc_t* prev;
	fn_cmp_key_t fn_cmp_key;
	void* val;
	
	if(!htable || !key)
	{
		errno = EINVAL;
		return NULL;
	}
	
	prev = &(((htable->content)[(*(htable->fn_hfunc))(key) % htable->size]));
	fn_cmp_key =htable->fn_cmp_key;

	for (hassoc = *prev; hassoc; hassoc = hassoc->next)
	{
		if((*fn_cmp_key)((hassoc->key),key))
		{
			*prev = hassoc->next;  
			val = (void*)hassoc->val;
			
			if(allocator_dealloc(htable->allocator,hassoc))
				return NULL;
				
			return val;
		}
		
		prev = &(hassoc->next);
	}
	
	
	errno = ESRCH;
	return NULL;
}

int
htable_erase(htable_t htable, const void* key)
{
	register hassoc_t hassoc;
	hassoc_t* prev;
	fn_cmp_key_t fn_cmp_key;
	void* val;
	int rv;
	
	if(!htable || !key)
		return EINVAL;
	 
	prev = &(((htable->content)[(*(htable->fn_hfunc))(key) % htable->size]));
	
	fn_cmp_key =htable->fn_cmp_key;

	for(hassoc = *prev; hassoc; hassoc = hassoc->next)
	{
		if((*fn_cmp_key)((hassoc->key),key))
		{
			*prev = hassoc->next;  
			val = (void*)hassoc->val;
			
			if((rv = allocator_dealloc(htable->allocator,hassoc)))
				return rv;
				
			if(htable->fn_finalize)
			{	
				if((rv = (*(htable->fn_finalize))(&val)))
					return rv;
			}
			
			return 0;
		}
		
		prev = &(hassoc->next);
	}
	
	return ESRCH;
}

int
htable_free(htable_t* htableptr)
{
	htable_t htable;
	int rv;
	
	if(!(*htableptr))
		return EINVAL;
	
	htable = (htable_t)(*htableptr);
	
	if(htable->fn_finalize)
	{
		hassoc_t* content;
		register hassoc_t hassoc;
		register int pos;
		int size;
		void* val;
	
		content = htable->content;
		size = htable->size;
	
		for(pos = 0; pos < size; pos++)
		{
			for(hassoc = content[pos]; hassoc; hassoc = hassoc->next)
			{
				val = (void*)hassoc->val;
				if((rv = (*(htable->fn_finalize))(&val)))
					return rv;
			}
		}
	}
	
	free(htable->content);
	
	if((rv = allocator_free(&(htable->allocator))))
		return rv;
		
	free(*htableptr);
	*htableptr = NULL;
	
	return 0;
}

int
htable_clear(htable_t htable)
{
	hassoc_t* content;
	register hassoc_t hassoc;
	register int pos;
	int size;
	void* val;
	int rv;
	
	if(!htable)
		return EINVAL;
	
	
	content = htable->content;
	size = htable->size;
	
	if(htable->fn_finalize)
	{
		for(pos = 0; pos < size; pos++)
		{
			for(hassoc = content[pos]; hassoc; hassoc = hassoc->next)
			{
				val = (void*)hassoc->val;
				if((rv = (*(htable->fn_finalize))(&val)))
					return rv;
			}
			
			content[pos] = NULL;
		}
	}
	else
	{
		for(pos = 0; pos < size; pos++)
			content[pos] = NULL;
	}
	
	return allocator_clear(htable->allocator);
}

int
htable_get_size(htable_t htable)
{
	
	if(!htable)
	{
		errno = EINVAL;
		return -1;
	}
	
	return allocator_get_size(htable->allocator);
}

int
htable_is_empty(htable_t htable)
{
	if(!htable)
	{
		errno = EINVAL;
		return 0;
	}
	
	return  allocator_get_size(htable->allocator);
}

int
htable_is_autodelete(htable_t htable)
{
	
	if(!htable)
	{
		errno = EINVAL;
		return 0;
	}
	
	return (NULL != htable->fn_finalize);
}

