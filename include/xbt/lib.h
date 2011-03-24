#ifndef _XBT_LIB_H
#define _XBT_LIB_H

#define MAX_FILL_PERCENT 80

SG_BEGIN_DECL()

typedef struct xbt_lib_ *xbt_lib_t;
typedef struct xbt_libelm_ *xbt_libelm_t;
typedef struct xbt_lib_cursor_ *xbt_lib_cursor_t;
typedef struct xbt_lib_cursor_ s_xbt_lib_cursor_t;

struct xbt_lib_cursor_ {
  xbt_libelm_t current;
  int line;
  xbt_lib_t lib;
};

typedef struct xbt_libelm_ {
  char *key;
  int key_len;
  unsigned int hash_code;
  xbt_libelm_t next;
  void *content;
} s_xbt_libelm_t;

typedef struct xbt_lib_ {
  int table_size;
  int count;
  int fill;
  int levels;
  void_f_pvoid_t *free_f; // This is actually a table
  xbt_libelm_t *table;    // This is actually a table
} s_xbt_lib_t;


/*####[ Prototypes ]#################################################*/
XBT_PUBLIC(void) xbt_lib_preinit(void);
XBT_PUBLIC(void) xbt_lib_postexit(void);
XBT_PUBLIC(xbt_lib_t) xbt_lib_new(void);
XBT_PUBLIC(void) xbt_lib_free(xbt_lib_t * lib);
XBT_PUBLIC(int) xbt_lib_add_level(xbt_lib_t lib, void_f_pvoid_t free_f); // Une fois qu'on a inséré un objet, on ne peut plus ajouter de niveau
XBT_PUBLIC(void) xbt_lib_set(xbt_lib_t lib, const char *name, int level, void *obj);
XBT_PUBLIC(void *) xbt_lib_get_or_null(xbt_lib_t lib, const char *name, int level);
XBT_PUBLIC(int) xbt_lib_length(xbt_lib_t lib);
XBT_PUBLIC(void) xbt_lib_reset(xbt_lib_t *lib);
XBT_PUBLIC(void) xbt_lib_cursor_step(xbt_lib_cursor_t cursor);
XBT_PUBLIC(int) xbt_lib_cursor_get_or_free(xbt_lib_cursor_t * cursor, char **key, void **data);
XBT_PUBLIC(void) xbt_lib_cursor_first(const xbt_lib_t lib, xbt_lib_cursor_t * cursor);
XBT_PUBLIC(unsigned int) xbt_lib_size(xbt_lib_t lib);

/** @def xbt_lib_foreach
    @hideinitializer */
#define xbt_lib_foreach(lib,cursor,key,data)                       \
  for (cursor=NULL, xbt_lib_cursor_first((lib),&(cursor)) ;        \
       xbt_lib_cursor_get_or_free(&(cursor),(char**)&(key),(void**)(&data));\
       xbt_lib_cursor_step(cursor) )

SG_END_DECL()
#endif                          /* _XBT_LIB_H */
