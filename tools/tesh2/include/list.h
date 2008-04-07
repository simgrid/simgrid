#ifndef __list_H
#define __list_H

#include <allocator.h>


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

#ifndef __LINK_T_DEFINED
typedef struct s_link_t
{
	void* item;						/* the item associated with the link								*/
	struct s_link_t* next;			/* address to the next link											*/
	struct s_link_t* prev;			/* address to the prev link											*/
}s_link_t,* link_t;
#define __LINK_T_DEFINED
#endif

typedef struct s_list
{
	void* item;						/* not used															*/	
	link_t next;					/* point to the last node of the list								*/
	link_t prev;					/* point to the first node of the list								*/
	fn_finalize_t fn_finalize;		/* not used															*/
	int size;					/* the number of node contained by the list							*/
	link_t cur;
	int pos;
}s_list_t,* list_t;

list_t
list_new(fn_finalize_t fn_finalize);

int
list_rewind(list_t list);

int
list_unwind(list_t list);

int
list_clear(list_t list);

int
list_free(list_t* list_ptr);

int
list_push_front(list_t list, void* item);

int
list_push_back(list_t list, void* item);

void*
list_pop_back(list_t list);

void*
list_pop_front(list_t list);

int
list_remove(list_t list, void* item);

int
list_get_size(list_t list);
int
list_contains(list_t list, void* item);

int
list_is_empty(list_t list);

int
list_is_autodelete(list_t list);

int
list_move_next(list_t list);

void*
list_get(list_t list);

void*
list_set(list_t list, void* item);

void*
list_get_at(list_t list, int pos);

void*
list_set_at(list_t list, int pos, void* item);

int
list_move_prev(list_t list);

int
list_seek(list_t list, int offset, int whence);

int
list_tell(list_t list);

int
list_getpos(list_t list, int* pos);

int
list_setpos(list_t list, int pos);


void*
list_get_front(list_t list);

void*
list_get_back(list_t list);

int 
list_insert_after(list_t list, void* what, void* where);

int 
list_insert_before(list_t list, void* what, void* where);


#ifdef __cplusplus
}
#endif


#endif /* !__list_H */
