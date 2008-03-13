#ifndef __ALLOCATOR_H
#define __ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif


#ifndef __FN_FINALIZE_T_DEFINED
typedef int (*fn_finalize_t)(void**);
#define __FN_FINALIZE_T_DEFINED
#endif

#ifndef __BYTE_T_DEFINED
typedef unsigned char byte;
#define __BYTE_T_DEFINED
#endif

/* forward declarations */
struct s_allocator;			/* allocator struct			*/
struct s_allocator_block;	/* allocator block struct	*/
struct s_allocator_node;	/* allocator node struct	*/


/* the allocator node type */
typedef struct s_allocator_node
{
	struct s_allocator_node* next;				/* the next node in the block											*/	
	struct s_allocator_block* block;			/* the block which contains the node									*/	
	int is_allocated;								/* if 1 the node is allocated (used)									*/
}s_allocator_node_t,* allocator_node_t;

/* the allocator block type	*/
typedef struct s_allocator_block
{
	struct s_allocator* allocator;			/* the allocator which contains the block								*/	
	struct s_allocator_block* next;			/* the next block														*/
}s_allocator_block_t,* allocator_block_t;

/* the allocator type */
typedef struct s_allocator
{
	allocator_block_t head;					/* the head of the allocator blocks										*/
	unsigned int block_capacity;			/* the capacity of the allocator blocks (nodes per block)				*/
	int capacity;							/* the current capacity of the allocator (nodes per block x block count)*/											
    int type_size;    						/* size of allocated type												*/
	int size;								/* current node (object) count											*/
	allocator_node_t free;					/* pointer to the next free node	 									*/
	allocator_node_t first;					/* pointer to the first free node										*/
	fn_finalize_t fn_finalize;	/* pointer to the function used to destroy the allocated objects		*/
}s_allocator_t,* allocator_t;

allocator_t
allocator_new(int block_capacity, int type_size,  fn_finalize_t fn_finalize);

int
allocator_free(allocator_t* allocator_ptr);

void* 
allocator_alloc(allocator_t allocator);

int
allocator_dealloc(allocator_t allocator,void* block);

int
allocator_get_size(allocator_t allocator);

int
allocator_get_capacity(allocator_t allocator);

int
allocator_get_type_size(allocator_t allocator);

int
allocator_get_block_capacity(allocator_t allocator);

int
allocator_clear(allocator_t allocator);

int
allocator_get_capacity_available(allocator_t allocator);

int
allocator_is_empty(allocator_t allocator);

int
allocator_is_full(allocator_t allocator);


#ifdef __cplusplus
}
#endif

#endif /* !__allocator_H */
