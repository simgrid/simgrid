#include <xalloc.h>
#include <htable.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <stdio.h>

#include <portable.h>
#include <xbt/xbt_os_thread.h>


#ifndef __DEFAULT_BLOCK_CAPACITY
#define __DEFAULT_BLOCK_CAPACITY	((int)512)
#endif

#ifndef __DEFAULT_TABLE_SIZE
#define __DEFAULT_TABLE_SIZE		((int)256)
#endif


#ifndef __BYTE_T_DEFINED
	typedef unsigned char byte;
	#define __BYTE_T_DEFINED
#endif


static int
_aborded = 0;

/*
static htable_t
_heap = NULL;

static xbt_os_mutex_t
_mutex = NULL;


static unsigned long 
hfunc(const void* key)
{
	return ((unsigned long)(void*)(unsigned long)key) >> 4;
}

static int
cmp_key(const void* k1, const void* k2)
{
	return (k1 == k2);
}
*/

int
xmalloc_mod_init(void)
{
	/*allocator_node_t cur, next;
	allocator_block_t block;
	int block_capacity, type_size, node_size;
	register int pos;
	
	if(_heap)
		return EALREADY;
		
	_mutex = xbt_os_mutex_init();
		
	
	if(!(_heap = (htable_t)calloc(1, sizeof(s_htable_t))))
		return -1;
	
	if(!(_heap->content = (hassoc_t*)calloc(__DEFAULT_TABLE_SIZE, sizeof(hassoc_t))))
	{
		free(_heap);
		return -1;
	}
	
	if(!(_heap->allocator = (allocator_t)calloc(1,sizeof(s_allocator_t))))
	{
		free(_heap->content);
		free(_heap);
		return -1;
	}

	_heap->allocator->block_capacity = __DEFAULT_BLOCK_CAPACITY;
	_heap->allocator->type_size = sizeof(s_hassoc_t);
	
	
	next = NULL;
	block_capacity = __DEFAULT_BLOCK_CAPACITY;
	type_size =  sizeof(s_hassoc_t);
	node_size = type_size + sizeof(s_allocator_node_t);
	
	if(!(block = (allocator_block_t)calloc(1,sizeof(s_allocator_block_t) + (block_capacity * node_size))))
	{
		free(_heap->content);
		free(_heap->allocator);
		free(_heap);
		return -1;
	}
		
	block->next = _heap->allocator->head;
	block->allocator =  _heap->allocator;
	_heap->allocator->head = block;
	
	cur = (allocator_node_t)(((byte*)(block + 1)) + ((block_capacity - 1) * node_size));
	
	for(pos = block_capacity - 1; pos >= 0; pos--, cur = (allocator_node_t)(((byte*)next) - node_size))
	{
		cur->next = next;
		cur->block = block;
		next = cur;
	}
	
	 _heap->allocator->free = _heap->allocator->first = next;
	
	 _heap->allocator->capacity += block_capacity;
	 _heap->allocator->fn_finalize = NULL;
   
	
	_heap->size = __DEFAULT_TABLE_SIZE;
	_heap->fn_hfunc = hfunc;
	_heap->fn_cmp_key = cmp_key;
	_heap->fn_finalize = NULL;
	
	*/
	return 0;
}

void
xabort(void)
{
	_aborded = 1;
	abort();
}

int
xmalloc_mod_exit(void)
{
	/*hassoc_t* content;
	register hassoc_t hassoc;
	allocator_block_t cur, next;
	register int pos;
	int size;
	void* val;
	
	if(_heap)
	{
		errno = EPERM;
		return -1;
	}
	
	if(!htable_is_empty(_heap))
	{
		if(!_aborded)
			fprintf(stderr,"WARNING : Memory leak detected - automaticaly release the memory...\n");
		else
			fprintf(stderr,"System aborted - Automaticaly release the memory...\n");	
	}
		

	content = _heap->content;
	size = _heap->size;

	for(pos = 0; pos < size; pos++)
	{
		for(hassoc = content[pos]; hassoc; hassoc = hassoc->next)
		{
			val = (void*)hassoc->val;
			if(xfree(&val) < 0)
				return -1;
		}
	}
	
	free(_heap->content);
	
	cur = _heap->allocator->head;
	
	while(cur)
	{
		next = cur->next;
		xfree(cur);
		cur = next;
	}
	
	
	free(_heap->allocator);
	
	free(_heap);
	_heap = NULL;
	
	xbt_os_mutex_destroy(_mutex);
	*/
	return 0;
}

void*
xmalloc(unsigned int size)
{
	byte* p;
	
	if(!(p = (byte*)calloc(size + 1, sizeof(byte))))
		return NULL;
	
	p[0] = XMAGIC;
	
	/*if(htable_set(_heap, p, p))
	{
		xfree(p);
		return NULL;
	}*/
	
	return p + 1;
}

void*
xcalloc(unsigned int count, unsigned int size)
{
	byte* p;
	
	if(!(p = (byte*)calloc((size * count) + 1, sizeof(byte))))
		return NULL;
	
	p[0] = XMAGIC;
	
	/*if(htable_set(_heap, p, p))
	{
		xfree(p);
		return NULL;
	}*/
	
	return p + 1;
}

int
xfree(void* ptr)
{
	byte* _ptr;
	
	if(!ptr)
	{
		errno = EINVAL;
		return -1;
	}
	
	_ptr = (byte*)ptr - 1;
	
	if(XMAGIC != _ptr[0])
	{
		errno = EINVAL;
		return -1;
	}
	
	/*if(!htable_lookup(_heap, _ptr))
		return -1;
	
	if(!htable_remove(_heap, _ptr))
		return -1;
	*/
	
	free(_ptr);
	
	return 0;
}

void*
xrealloc(void *ptr, unsigned int size)
{
	byte* _ptr,* _ptr_r ;
	
	if(!ptr)
	{
		/* If ptr is NULL, realloc() is identical to a call
     	 * to malloc() for size byte
     	 */
		 _ptr = (byte*)calloc(size + 1, sizeof(byte));
	
		if(!_ptr)
			return NULL;
	
		_ptr[0] = XMAGIC;
		
		return _ptr + 1;
	}
	
	if(ptr && !size)
	{
		/* If size is zero and ptr is not NULL, the allocated 
		 * object is freed.
		 */
		xfree(ptr);
		return NULL;
	}
	
	_ptr = (byte*)ptr - 1;
	
	if(XMAGIC != _ptr[0])
	{
		errno = EINVAL;
		return NULL;
	}
	
	if((_ptr_r = realloc(_ptr, size)))
		return _ptr_r + 1;
	
	return NULL;
}

char*
xstrdup(const char* s1)
{
	char* d1;
	
	if(!s1)
		return NULL;
	
	if((d1 = xmalloc((unsigned int)strlen(s1) +1 )))
		strcpy(d1, s1);
	
	return d1;
}

