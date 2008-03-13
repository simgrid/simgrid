#ifndef __lstrings_H
#define __lstrings_H


#ifdef __cplusplus
extern "C" {
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

typedef struct s_lstrings
{
	const char* item;				/* not used															*/	
	link_t next;					/* point to the last node of the lstrings						*/
	link_t prev;					/* point to the first node of the lstrings						*/
	int size;						/* the number of node contained by the lstrings					*/
	link_t cur;
	int pos;
}s_lstrings_t,* lstrings_t;

lstrings_t
lstrings_new(void);

int
lstrings_rewind(lstrings_t lstrings);

int
lstrings_unwind(lstrings_t lstrings);

int
lstrings_clear(lstrings_t lstrings);

int
lstrings_free(lstrings_t* lstrings_ptr);

int
lstrings_push_front(lstrings_t lstrings, const char* string);

int
lstrings_push_back(lstrings_t lstrings, const char* string);

const char*
lstrings_pop_back(lstrings_t lstrings);

const char*
lstrings_pop_front(lstrings_t lstrings);

int
lstrings_remove(lstrings_t lstrings, const char* string);

int
lstrings_get_size(lstrings_t lstrings);
int
lstrings_contains(lstrings_t lstrings, const char* string);

int
lstrings_is_empty(lstrings_t lstrings);

int
lstrings_move_next(lstrings_t lstrings);

const char*
lstrings_get(lstrings_t lstrings);

const char*
lstrings_set(lstrings_t lstrings, const char* string);

const char*
lstrings_get_at(lstrings_t lstrings, int pos);

const char*
lstrings_set_at(lstrings_t lstrings, int pos, const char* string);

int
lstrings_move_prev(lstrings_t lstrings);

int
lstrings_seek(lstrings_t lstrings, int offset, int whence);

int
lstrings_tell(lstrings_t lstrings);

int
lstrings_getpos(lstrings_t lstrings, int* pos);

int
lstrings_setpos(lstrings_t lstrings, int pos);

const char*
lstrings_get_front(lstrings_t lstrings);

const char*
lstrings_get_back(lstrings_t lstrings);

int 
lstrings_insert_after(lstrings_t lstrings, const char* what, const char* where);

int 
lstrings_insert_before(lstrings_t lstrings, const char* what, const char* where);

char**
lstrings_to_cstr(lstrings_t lstrings);


#ifdef __cplusplus
}
#endif


#endif /* !__lstrings_H */
