#ifndef __HTABLE
#define __HTABLE


#include <allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __FN_FINALIZE_T_DEFINED
typedef int (*fn_finalize_t)(void**);
#define __FN_FINALIZE_T_DEFINED
#endif


/* 
 * pointer to a compare key function.
 * the function takes two items to compare and returns true if they are equal.
 */
#ifndef __FN_CMP_KEY_T_DEFINED
typedef int (*fn_cmp_key_t)(const void*, const void*);
#define __FN_CMP_KEY_T_DEFINED
#endif

/* 
 * pointer to a hash function.
 * the function take the value to compute the hash value that it returns.
 */ 
#ifndef __FN_HFUNC_T_DEFINED
typedef unsigned int (*fn_hfunc_t)(const void*);
#define __FN_HFUNC_T_DEFINED
#endif

typedef struct s_hassoc
{
		struct s_hassoc* next;
		const void* key;
		const void* val;
}s_hassoc_t,* hassoc_t;

typedef struct s_htable
{
	hassoc_t* content;			/* the hache table content									*/
	int size;			/* the size of the hash table							*/
	allocator_t allocator;		/* the allocator used to allocate the associations		*/
	fn_hfunc_t fn_hfunc;			/* a pointer to the hash function to use				*/
	fn_cmp_key_t fn_cmp_key;	/* a pointer to the function used to fn_cmp_key the key	*/
	fn_finalize_t fn_finalize;
}s_htable_t,* htable_t;





htable_t
htable_new(
				int block_capacity,							/* the block capacity of the blocks of the allocator used by the htable	*/ 
				int size,							/* the size of the table size											*/ 
				fn_hfunc_t fn_hfunc,							/* the pointer to the function to use									*/ 
				fn_cmp_key_t fn_cmp_key,					/* the pointer to the function used to fn_cmp_key the keys of the assocs	*/ 
				fn_finalize_t fn_finalize		/* the pointer to the function used to destroy the values of the assocs	*/
			);

int
htable_set(htable_t htable, const void* key, const void* val);

void*
htable_lookup(htable_t htable, const void* key);

void*
htable_remove(htable_t htable, const void* key);

int
htable_erase(htable_t htable, const void* key);

int
htable_free(htable_t* htableptr);

int
htable_clear(htable_t htable);

int
htable_get_size(htable_t htable);

int
htable_is_empty(htable_t htable);

int
htable_is_autodelete(htable_t htable);

#ifdef __cplusplus
}
#endif

#endif /* !__HTABLE */

