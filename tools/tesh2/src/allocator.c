
#include <allocator.h>

#include <errno.h>
#include <string.h>	/* memset			*/
#include <stdlib.h>	/* calloc()	free()	*/


static int
resize(allocator_t allocator)
{
	allocator_node_t cur, next;
	allocator_block_t block;
	int block_capacity, type_size, node_size;
	register int pos;
	
	if(!allocator)
		return EINVAL;
	
	next = NULL;
	block_capacity = allocator->block_capacity;
	type_size = allocator->type_size;
	node_size = type_size + sizeof(s_allocator_node_t);
	
	if(!(block = (allocator_block_t)calloc(1,sizeof(s_allocator_block_t) + (block_capacity * node_size))))
		return errno;
		
	/* update the first block of the allocator */
	block->next = allocator->head;
	block->allocator = allocator;
	allocator->head = block;
	
	/* move to the last node of the block. */
	cur = (allocator_node_t)(((byte*)(block + 1)) + ((block_capacity - 1) * node_size));
	
	/* initialize all the nodes of the new block */
	for(pos = block_capacity - 1; pos >= 0; pos--, cur = (allocator_node_t)(((byte*)next) - node_size))
	{
		cur->next = next;
		cur->block = block;
		next = cur;
	}
	
	/* allocator->free pointed now on the first node of the new bloc. */
	allocator->free = allocator->first = next;
	/* update the allocator capacity. */
	allocator->capacity += block_capacity;
	
	return 0;	
}

allocator_t
allocator_new(int block_capacity, int type_size, fn_finalize_t fn_finalize)
{
	allocator_t allocator;
	
	if((block_capacity <= 0) || (type_size <= 0))
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!(allocator = (allocator_t)calloc(1,sizeof(s_allocator_t))))
		return NULL;

	/* updates allocator properties */
	allocator->block_capacity = block_capacity;
	allocator->type_size = type_size;
	
	/* first block allocation */
	
	if((errno = resize(allocator)))
	{
		free(allocator);
		return NULL;
	}
	
	allocator->fn_finalize = fn_finalize;
    return allocator;
}

int
allocator_free(allocator_t* allocator_ptr)
{
	allocator_block_t cur, next;
	allocator_node_t node;
	allocator_t allocator;
	int pos, node_size;
	fn_finalize_t fn_finalize;
	void* type;
	
	if(!(*allocator_ptr))
		return EINVAL;
	
	
	allocator = *allocator_ptr;
	cur = allocator->head;
	
	if(allocator->fn_finalize)
	{
		fn_finalize = allocator->fn_finalize;
		node_size = allocator->type_size + sizeof(s_allocator_node_t);
		
		while(cur)
		{
		
			/* type points to the first node */
			node = (allocator_node_t)(((byte*)cur) + sizeof(s_allocator_block_t));
			
			if(node->is_allocated)
			{
				type = (void*)(((byte*)node) +  sizeof(s_allocator_node_t));
			
				/* apply the fn_finalize function to the first type */
				
				if((errno = (*fn_finalize)(&type)))
					return errno;
			}
			
			/*clear all the other types */
			for(pos = 1; pos < allocator->block_capacity; pos++)
			{
				node = (allocator_node_t)(((byte*)node) + node_size);
				
				if(node->is_allocated)
				{
					type = (void*)(((byte*)node) +  sizeof(s_allocator_node_t));
				
					/* apply the fn_finalize function to the first type */
					
					if((errno = (*fn_finalize)(&type)))
						return errno;
				}
			}
			
			next = cur->next;
			free(cur);
			cur = next;
		}
	}
	else
	{
		while(cur)
		{
			next = cur->next;
			free(cur);
			cur = next;
		}
	}
	
	free(*allocator_ptr);
	*allocator_ptr = NULL;
	
	return 0;
}

void*
allocator_alloc(allocator_t allocator)
{
    allocator_node_t node;

    if(!allocator)
    {
    	errno = EINVAL;
    	return NULL;
    }
    
	/* all allocator memory is used, allocate a new block */
	if(!(allocator->free))
		if(resize(allocator))
			return NULL;
	
	node = allocator->free;
	node->is_allocated = 1;
	
	allocator->free  = allocator->free->next;
	allocator->size++;
	
	return (void*)(((byte*)node) + sizeof(s_allocator_node_t));
}

int
allocator_dealloc(allocator_t allocator, void* block)
{
	allocator_node_t node;
	
	if(!allocator || !block)
    	return EINVAL;
	
	if(allocator->fn_finalize)
	{
		if((errno = (*(allocator->fn_finalize))(&block)))
			return errno;
			
		memset(block, 0, allocator->type_size);
		node->is_allocated = 0;
	}
	
	/* get the node address. */
	node = (allocator_node_t)(((byte*)block) - sizeof(s_allocator_node_t)); 
	
	/* the node becomes the free node and the free node become the next free node.*/
	node->next = allocator->free;
	allocator->free = node;
	allocator->size--;
	
	return 0;
}

int
allocator_get_size(allocator_t allocator)
{
	if(!allocator)
    {
    	errno = EINVAL;
    	return -1;
    }
    
    return allocator->size;
}

int
allocator_get_capacity(allocator_t allocator)
{
	if(!allocator)
    {
    	errno = EINVAL;
    	return -1;
    }
    
   return allocator->capacity;
}

int
allocator_get_type_size(allocator_t allocator)
{

	if(NULL == allocator)
    {
    	errno = EINVAL;
    	return -1;
    }
    
   return allocator->type_size;
}

int
allocator_is_empty(allocator_t allocator)
{
	if(NULL == allocator)
    {
    	errno = EINVAL;
    	return 0;
    }
    
    return !(allocator->size);
}


int
allocator_is_full(allocator_t allocator)
{
	if(NULL == allocator)
    {
    	errno = EINVAL;
    	return 0;
    }
    
    return (allocator->size == allocator->capacity);
}

int
allocator_get_block_capacity(allocator_t allocator)
{
		
	if(!allocator)
    {
    	errno = EINVAL;
    	return -1;
    }
    
   return allocator->block_capacity;
}

int
allocator_get_capacity_available(allocator_t allocator)
{
	if(!allocator)
    {
    	errno = EINVAL;
    	return -1;
    }
    
	return (allocator->capacity - allocator->size);
}

int
allocator_clear(allocator_t allocator)
{
	allocator_block_t cur;
	allocator_node_t node;
	
	int block_capacity, node_size, type_size;
	fn_finalize_t fn_finalize;
	void* type;
	register int pos;
	
	
	if(!allocator)
    	return EINVAL;
    
    
    if(allocator->fn_finalize)
	{
		fn_finalize = allocator->fn_finalize;
		block_capacity = allocator->block_capacity;
		type_size = allocator->type_size;
		node_size = type_size + sizeof(s_allocator_node_t);
		
		cur = allocator->head;
		 
		while(cur)
		{
			/* type points to the first node */
			node = (allocator_node_t)(((byte*)cur) + sizeof(s_allocator_block_t));
			
			if(node->is_allocated)
			{
			
				type = (void*)(((byte*)node) +  sizeof(s_allocator_node_t));
			
				/* apply the fn_finalize function to the first type */
				
				if((errno = (*fn_finalize)(&type)))
					return errno;
				
				memset(type, 0, type_size);
				node->is_allocated = 0;
			}
			
			/*clear all the other types */
			for(pos = 1; pos < block_capacity; pos++)
			{
				node = (allocator_node_t)(((byte*)node) + node_size);
				
				if(node->is_allocated)
				{
					type = (void*)(((byte*)node) +  sizeof(s_allocator_node_t));
				
					/* apply the fn_finalize function to the first type */
					
					if((errno = (*fn_finalize)(&type)))
						return errno;
				
					memset(type, 0, type_size);
					node->is_allocated = 0;
				}
			}
		}
		
		cur = cur->next;
	}
	
    allocator->free = allocator->first;
    allocator->size = 0;
    
    return 0;
}
