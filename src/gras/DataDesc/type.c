/* gs_type.c */

#include "DataDesc/gs_private.h"
GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(NDR_type_creation,NDR);

/*......................................................................
 * Type
 */

/* alloc/free */
static
struct s_gs_type *
_gs_type_alloc(void) {
        struct s_gs_type 	*p_type = NULL;
        p_type = malloc(sizeof(struct s_gs_type));
        return p_type;
}

static
struct s_gs_type *
_gs_type_new(struct s_gs_type_bag	*p_bag,
             struct s_gs_connection	*p_connection,
             const char			*name) {

        struct s_gs_type_bag_ops	*bag_ops = p_bag->bag_ops;
        struct s_gs_type 	 	*p_type  = NULL;

        p_type = _gs_type_alloc();

        p_type->code = -1;
        p_type->name = strdup(name);

        p_type->size                    = 0;
        p_type->alignment               = 0;
        p_type->aligned_size            = 0;
        p_type->category_code           = e_gs_type_cat_undefined;
        p_type->category.undefined_data = NULL;
        p_type->before_callback		= NULL;
        p_type->after_callback		= NULL;

        bag_ops->store_type(p_bag, p_connection, p_type);

        return p_type;
}





/*......................................................................
 * Elemental
 */
static
struct s_gs_cat_elemental *
_gs_elemental_cat_alloc(void) {
        struct s_gs_cat_elemental	*p_elemental	= NULL;

        p_elemental = malloc(sizeof (struct s_gs_cat_elemental));

        return p_elemental;
}

static
struct s_gs_cat_elemental *
_gs_elemental_cat_new(void) {
        struct s_gs_cat_elemental	*p_elemental	= NULL;

        p_elemental = _gs_elemental_cat_alloc();
        p_elemental->encoding = e_gs_elemental_encoding_undefined;

        return p_elemental;
}

struct s_gs_type *
gs_type_new_elemental_with_callback(struct s_gs_type_bag	*p_bag,
                                    struct s_gs_connection	*p_connection,
                                    const char                         *name,
                                    enum e_gs_elemental_encoding  encoding,
                                    long int                      size,

                                    void (*callback)(void		*vars,
                                                     struct s_gs_type	*p_type,
                                                     void		*data)) {

        struct s_gs_type 		*p_type		= NULL;
        struct s_gs_cat_elemental	*p_elemental	= NULL;

	if (callback)
	  DEBUG1("%s",__FUNCTION__);
        p_elemental = _gs_elemental_cat_new();
        p_elemental->encoding = encoding;

        p_type = _gs_type_new(p_bag, p_connection, name);

        p_type->size = size > 0?size:0;

        if (size > 0) {
                long int sz   = size;
                long int mask = sz;

                while ((sz >>= 1)) {
                        mask |= sz;
                }

                if (size & (mask >> 1)) {
                        long int alignment	= 0;
                        long int aligned_size	= 0;

                        alignment	= (size & ~(mask >> 1)) << 1;
                        if (alignment < 0)
                                GS_FAILURE("elemental type is too large");

                        aligned_size	= aligned(size, alignment);
                        if (aligned_size < 0)
                                GS_FAILURE("elemental type is too large");

                        p_type->alignment	= alignment;
                        p_type->aligned_size	= aligned_size;

                } else {
                        long int alignment	= size & ~(mask >> 1);
                        long int aligned_size	= aligned(size, alignment);

                        p_type->alignment	= alignment;
                        p_type->aligned_size	= aligned_size;
                }

        } else {
                p_type->alignment	= 0;
                p_type->aligned_size	= 0;
        }

        p_type->category_code           = e_gs_type_cat_elemental;
        p_type->category.elemental_data = p_elemental;

        p_type->before_callback		= callback;

        return p_type;
}

struct s_gs_type *
gs_type_new_elemental(struct s_gs_type_bag	*p_bag,
                      struct s_gs_connection	*p_connection,
                      const char                         *name,
                      enum e_gs_elemental_encoding  encoding,
                      long int                      size) {
	DEBUG1("%s",__FUNCTION__);
        return gs_type_new_elemental_with_callback(p_bag, p_connection, name, encoding, size, NULL);
}


struct s_gs_type *
gs_type_new_unsigned_integer_elemental_with_callback(struct s_gs_type_bag	*p_bag,
                                                     struct s_gs_connection	*p_connection,
                                                     const char	*name,
                                                     long int	 size,

                                                     void (*callback)(void		*vars,
                                                                      struct s_gs_type	*p_type,
                                                                      void		*data)) {
	if (callback)
	  DEBUG1("%s",__FUNCTION__);
        return gs_type_new_elemental_with_callback(p_bag, p_connection, name,
                                                   e_gs_elemental_encoding_unsigned_integer,
                                                   size,
                                                   callback);
}

struct s_gs_type *
gs_type_new_unsigned_integer_elemental(struct s_gs_type_bag	*p_bag,
                                       struct s_gs_connection	*p_connection,
                                       const char *name,
                                       long int size) {
	DEBUG1("%s",__FUNCTION__);
        return gs_type_new_unsigned_integer_elemental_with_callback(p_bag, p_connection, name, size, NULL);
}


struct s_gs_type *
gs_type_new_signed_integer_elemental_with_callback(struct s_gs_type_bag	*p_bag,
                                                   struct s_gs_connection	*p_connection,
                                                   const char		*name,
                                                   long int	 size,

                                                   void (*callback)(void		*vars,
                                                                    struct s_gs_type	*p_type,
                                                                    void		*data)) {
	if (callback)
	  DEBUG1("%s",__FUNCTION__);
        return gs_type_new_elemental_with_callback(p_bag, p_connection, name,
                                                   e_gs_elemental_encoding_signed_integer,
                                                   size,
                                                   callback);
}

struct s_gs_type *
gs_type_new_signed_integer_elemental(struct s_gs_type_bag	*p_bag,
                                     struct s_gs_connection	*p_connection,
                                     const char	*name,
                                     long int	 size) {
	DEBUG1("%s",__FUNCTION__);
        return gs_type_new_signed_integer_elemental_with_callback(p_bag, p_connection, name, size, NULL);
}


struct s_gs_type *
gs_type_new_floating_point_elemental_with_callback(struct s_gs_type_bag	*p_bag,
                                                   struct s_gs_connection	*p_connection,
                                                   const char		*name,
                                                   long int	 size,

                                                   void (*callback)(void		*vars,
                                                                    struct s_gs_type	*p_type,
                                                                    void		*data)) {
	DEBUG1("%s",__FUNCTION__);
        return gs_type_new_elemental_with_callback(p_bag, p_connection, name,
                                                   e_gs_elemental_encoding_floating_point,
                                                   size,
                                                   callback);

}

struct s_gs_type *
gs_type_new_floating_point_elemental(struct s_gs_type_bag	*p_bag,
                                     struct s_gs_connection	*p_connection,
                                     const char 	*name,
                                     long int	size) {
	DEBUG1("%s",__FUNCTION__);
        return gs_type_new_floating_point_elemental_with_callback(p_bag, p_connection, name, size, NULL);
}





/*......................................................................
 * Struct
 */
static
struct s_gs_cat_struct *
_gs_struct_cat_alloc(void) {
        struct s_gs_cat_struct	*p_struct	= NULL;
        p_struct = malloc(sizeof (struct s_gs_cat_struct));
        return p_struct;
}

static
struct s_gs_cat_struct *
_gs_struct_cat_new(void) {
        struct s_gs_cat_struct	*p_struct	= NULL;

        p_struct = _gs_struct_cat_alloc();

        p_struct->nb_fields	= 0;
        p_struct->field_array	= NULL;

        return p_struct;
}

struct s_gs_type *
gs_type_new_struct_with_callback(struct s_gs_type_bag	*p_bag,
                                 struct s_gs_connection	*p_connection,
                                 const char *name,

                                 void (*before_callback)(void		*vars,
                                                         struct s_gs_type	*p_type,
                                                         void		*data),

                                 void (*after_callback)(void		*vars,
                                                        struct s_gs_type	*p_type,
                                                        void		*data)) {
        struct s_gs_type 		*p_type		= NULL;
        struct s_gs_cat_struct		*p_struct	= NULL;

	if (before_callback || after_callback) 
	DEBUG1("%s",__FUNCTION__);
        p_struct = _gs_struct_cat_new();

        p_type   = _gs_type_new(p_bag, p_connection, name);

        p_type->size			= 0;
        p_type->alignment		= 0;
        p_type->aligned_size		= 0;
        p_type->category_code		= e_gs_type_cat_struct;
        p_type->category.struct_data	= p_struct;
        p_type->before_callback		= before_callback;
        p_type->after_callback		= after_callback;

        return p_type;
}

struct s_gs_type *
gs_type_new_struct(struct s_gs_type_bag	*p_bag,
                   struct s_gs_connection	*p_connection,
                   const char *name) {
	DEBUG2("%s(%s)",__FUNCTION__,name);
        return gs_type_new_struct_with_callback(p_bag, p_connection, name, NULL, NULL);
}


static
struct s_gs_cat_struct_field *
_gs_struct_field_cat_alloc(void) {
        struct s_gs_cat_struct_field	*p_struct_field	= NULL;
        p_struct_field = malloc(sizeof (struct s_gs_cat_struct_field));
        return p_struct_field;
}

static
struct s_gs_cat_struct_field *
_gs_struct_field_cat_new(const char             *name,
                         long int          offset,
                         struct s_gs_type *p_field_type,

                         void (*before_callback)(void			*vars,
                                                 struct s_gs_type	*p_type,
                                                 void			*data),

                         void (*after_callback)(void			*vars,
                                                struct s_gs_type	*p_type,
                                                void			*data)) {

        struct s_gs_cat_struct_field	*p_struct_field	= NULL;

        if (p_field_type->size < 0)
                GS_FAILURE("variable length field not allowed in structure");

        p_struct_field = _gs_struct_field_cat_alloc();

        p_struct_field->name		= strdup(name);
        p_struct_field->offset		= aligned(offset, p_field_type->alignment);
        p_struct_field->code		= p_field_type->code;
        p_struct_field->before_callback	= before_callback;
        p_struct_field->after_callback	= after_callback;

        return p_struct_field;
}

void
gs_type_struct_append_field_with_callback(struct s_gs_type	*p_struct_type,
                                          const char			*name,
                                          struct s_gs_type	*p_field_type,

                                          void (*before_callback)(void			*vars,
                                                                  struct s_gs_type	*p_type,
                                                                  void			*data),

                                          void (*after_callback)(void			*vars,
                                                                 struct s_gs_type	*p_type,
                                                                 void			*data)) {

        struct s_gs_cat_struct		*p_struct	= NULL;
        int                              field_num      =   -1;
        struct s_gs_cat_struct_field    *p_field        = NULL;

	if (before_callback || after_callback) 
	  DEBUG1("%s",__FUNCTION__);

        if (p_field_type->size < 0)
                GS_FAILURE("field size must be statically known");

        p_struct = p_struct_type->category.struct_data;

        if ((field_num = p_struct->nb_fields++)) {
                p_struct->field_array	= realloc(p_struct->field_array, p_struct->nb_fields * sizeof (struct s_gs_cat_struct_field *));
        } else {
                p_struct->field_array	= malloc(sizeof (struct s_gs_cat_struct_field *));
        }

        p_field	= _gs_struct_field_cat_new(name, p_struct_type->size, p_field_type, before_callback, after_callback);
        p_struct->field_array[field_num]	= p_field;

        p_struct_type->size		= p_field->offset + p_field_type->size;
        p_struct_type->alignment	= max(p_struct_type->alignment, p_field_type->alignment);
        p_struct_type->aligned_size	= aligned(p_struct_type->size, p_struct_type->alignment);
}

void
gs_type_struct_append_field(struct s_gs_type	*p_struct_type,
                            const char		*name,
                            struct s_gs_type	*p_field_type) {
	DEBUG1("%s",__FUNCTION__);
        gs_type_struct_append_field_with_callback(p_struct_type, name, p_field_type, NULL, NULL);
}




/*......................................................................
 * Union
 */
static
struct s_gs_cat_union *
_gs_union_cat_alloc(void) {
        struct s_gs_cat_union	*p_union	= NULL;
        p_union = malloc(sizeof (struct s_gs_cat_union));
        return p_union;
}

static
struct s_gs_cat_union *
_gs_union_cat_new(void) {
        struct s_gs_cat_union	*p_union	= NULL;

        p_union = _gs_union_cat_alloc();

        p_union->nb_fields	= 0;
        p_union->field_array	= NULL;

        return p_union;
}

struct s_gs_type *
gs_type_new_union_with_callback(struct s_gs_type_bag	*p_bag,
                                struct s_gs_connection	*p_connection,
                                const char *name,

                                int (*field_callback)(void		*vars,
                                                       struct s_gs_type	*p_type,
                                                       void		*data),

                                void (*after_callback)(void		*vars,
                                                       struct s_gs_type	*p_type,
                                                       void		*data)) {
        struct s_gs_type 		*p_type		= NULL;
        struct s_gs_cat_union		*p_union	= NULL;

        if (!field_callback
            &&
            (!p_connection||p_connection->direction != e_gs_connection_direction_incoming))
                GS_FAILURE("attempt to create a union type without discriminant");

        p_union	= _gs_union_cat_new();

        p_type	= _gs_type_new(p_bag, p_connection, name);

        p_type->size			= 0;
        p_type->alignment		= 0;
        p_type->aligned_size		= 0;
        p_type->category_code		= e_gs_type_cat_union;
        p_type->category.union_data	= p_union;
        p_type->before_callback		= NULL;

        p_union->callback		= field_callback;
        p_type->after_callback		= after_callback;

        return p_type;
}

struct s_gs_type *
gs_type_new_union(struct s_gs_type_bag	*p_bag,
                  struct s_gs_connection	*p_connection,
                  const char *name,

                  int (*field_callback)(void			*vars,
                                         struct s_gs_type	*p_type,
                                         void			*data)) {
        return gs_type_new_union_with_callback(p_bag, p_connection, name, field_callback, NULL);
}


static
struct s_gs_cat_union_field *
_gs_union_field_cat_alloc(void) {
        struct s_gs_cat_union_field	*p_union_field	= NULL;
        p_union_field = malloc(sizeof (struct s_gs_cat_union_field));
        return p_union_field;
}

static
struct s_gs_cat_union_field *
_gs_union_field_cat_new(const char			*name,
                        struct s_gs_type	*p_field_type,

                        void (*before_callback)(void			*vars,
                                                struct s_gs_type	*p_type,
                                                void			*data),

                        void (*after_callback)(void			*vars,
                                               struct s_gs_type	*p_type,
                                               void			*data)) {

        struct s_gs_cat_union_field	*p_union_field	= NULL;

        if (p_field_type->size < 0)
                GS_FAILURE("variable length field not allowed in structure");

        p_union_field = _gs_union_field_cat_alloc();

        p_union_field->name		= strdup(name);
        p_union_field->code		= p_field_type->code;
        p_union_field->before_callback	= before_callback;
        p_union_field->after_callback	= after_callback;

        return p_union_field;
}

void
gs_type_union_append_field_with_callback(struct s_gs_type	*p_union_type,
                                         const char			*name,
                                         struct s_gs_type	*p_field_type,

                                         void (*before_callback)(void			*vars,
                                                                 struct s_gs_type	*p_type,
                                                                 void			*data),

                                         void (*after_callback)(void			*vars,
                                                                struct s_gs_type	*p_type,
                                                                void			*data)) {

        struct s_gs_cat_union		*p_union	= NULL;
        int                              field_num      =   -1;
        struct s_gs_cat_union_field    *p_field        = NULL;

        if (p_field_type->size < 0)
                GS_FAILURE("field size must be statically known");

        p_union = p_union_type->category.union_data;

        if ((field_num = p_union->nb_fields++)) {
                p_union->field_array	= realloc(p_union->field_array, p_union->nb_fields * sizeof (struct s_gs_cat_union_field *));
        } else {
                p_union->field_array	= malloc(sizeof (struct s_gs_cat_union_field *));
        }

        p_field	= _gs_union_field_cat_new(name, p_field_type, before_callback, after_callback);
        p_union->field_array[field_num]	= p_field;

        p_union_type->size		= max(p_union_type->size, p_field_type->size);
        p_union_type->alignment		= max(p_union_type->alignment, p_field_type->alignment);
        p_union_type->aligned_size	= aligned(p_union_type->size, p_union_type->alignment);
}

void
gs_type_union_append_field(struct s_gs_type	*p_union_type,
                           const char		*name,
                           struct s_gs_type	*p_field_type) {
        gs_type_union_append_field_with_callback(p_union_type, name, p_field_type, NULL, NULL);
}




/*......................................................................
 * Ref
 */
static
struct s_gs_cat_ref *
_gs_ref_cat_alloc(void) {
        struct s_gs_cat_ref	*p_ref	= NULL;
        p_ref = malloc(sizeof (struct s_gs_cat_ref));
        return p_ref;
}

static
struct s_gs_cat_ref *
_gs_ref_cat_new(void) {
        struct s_gs_cat_ref	*p_ref	= NULL;

        p_ref		= _gs_ref_cat_alloc();
        p_ref->code	= 0;
        p_ref->callback	= NULL;

        return p_ref;
}

struct s_gs_type *
gs_type_new_ref_with_callback(struct s_gs_type_bag	*p_bag,
                              struct s_gs_connection	*p_connection,
                              const char		*name,
                              struct s_gs_type	*p_referenced_type,

                              int (*type_callback)(void		*vars,
                                                   struct s_gs_type	*p_type,
                                                   void		*data),

                              void (*after_callback)(void		*vars,
                                                     struct s_gs_type	*p_type,
                                                     void		*data)) {
        struct s_gs_type 		*p_type		= NULL;
        struct s_gs_cat_ref		*p_ref		= NULL;

        p_ref		= _gs_ref_cat_new();

        if (!p_referenced_type
            &&
            !type_callback
            &&
            (!p_connection||p_connection->direction != e_gs_connection_direction_incoming))
                GS_FAILURE("attempt to declare a generic ref without discriminant");

        p_ref->code	= p_referenced_type?p_referenced_type->code:-1;
        p_ref->callback = type_callback;

        p_type		= _gs_type_new(p_bag, p_connection, name);

        {
                struct s_gs_type_bag_ops	*bag_ops	= p_bag->bag_ops;
                struct s_gs_type		*p_pointer_type = NULL;

                p_pointer_type = bag_ops->get_type_by_name(p_bag, p_connection, "data pointer");
                p_type->size		= p_pointer_type->size;
                p_type->alignment	= p_pointer_type->alignment;
                p_type->aligned_size	= p_pointer_type->aligned_size;
        }

        p_type->category_code		= e_gs_type_cat_ref;
        p_type->category.ref_data	= p_ref;

        p_type->before_callback	= NULL;
        p_type->after_callback	= after_callback;

        return p_type;
}

struct s_gs_type *
gs_type_new_ref(struct s_gs_type_bag	*p_bag,
                struct s_gs_connection	*p_connection,
                const char		 *name,
                struct s_gs_type *p_referenced_type) {
        return gs_type_new_ref_with_callback(p_bag, p_connection, name, p_referenced_type, NULL, NULL);
}




/*......................................................................
 * Array
 */
static
struct s_gs_cat_array *
_gs_array_cat_alloc(void) {
        struct s_gs_cat_array	*p_array	= NULL;
        p_array = malloc(sizeof (struct s_gs_cat_array));
        return p_array;
}

static
struct s_gs_cat_array *
_gs_array_cat_new(void) {
        struct s_gs_cat_array	*p_array	= NULL;

        p_array			= _gs_array_cat_alloc();
        p_array->code		=    0;
        p_array->element_count	=    0;

        return p_array;
}

struct s_gs_type *
gs_type_new_array_with_callback(struct s_gs_type_bag	*p_bag,
                                struct s_gs_connection	*p_connection,
                                const char		   *name,
                                long int 	    size,
                                struct s_gs_type *p_array_element_type,

                                long int (*size_callback)(void		*vars,
                                                          struct s_gs_type	*p_type,
                                                          void		*data),

                                void (*after_callback)(void		*vars,
                                                       struct s_gs_type	*p_type,
                                                       void		*data)) {

        struct s_gs_type 		*p_type		= NULL;
        struct s_gs_cat_array		*p_array	= NULL;

        p_array			= _gs_array_cat_new();
        p_array->code		= p_array_element_type->code;
        p_array->element_count	= size;

        p_type = _gs_type_new(p_bag, p_connection, name);

        if (size <= 0) {
                if (size < 0 && !size_callback
                    &&
                    (!p_connection||p_connection->direction != e_gs_connection_direction_incoming))
                        GS_FAILURE("attempt to construct a variable sized array with no callback");

                p_type->size = size;
        } else {
                p_type->size = size * p_array_element_type->aligned_size;
        }

        p_type->alignment    = p_array_element_type->alignment;
        p_type->aligned_size = size;

        p_type->category_code     = e_gs_type_cat_array;
        p_type->category.array_data = p_array;

        p_array->callback = size_callback;

        p_type->before_callback = NULL;
        p_type->after_callback  = after_callback;

        return p_type;
}

struct s_gs_type *
gs_type_new_array(struct s_gs_type_bag	*p_bag,
                  struct s_gs_connection	*p_connection,
                  const char		   *name,
                  long int 	    size,
                  struct s_gs_type *p_array_element_type) {
        return gs_type_new_array_with_callback(p_bag, p_connection, name, size, p_array_element_type, NULL, NULL);
}





/*......................................................................
 * Type structure bootstrap
 */
static
long int
_strlen_cb(void			*p_vars,
           struct s_gs_type	*p_type,
           void			*data) {

        (void)p_vars;
        (void)p_type;

        return 1+(long int)strlen(data);
}

static
void
_category_code_push_cb(void		*p_vars,
                       struct s_gs_type	*p_type,
                       void		*data) {
        gs_vars_push(p_vars, p_type, "_category_code", data);
}

static
int
_category_code_pop_cb(void		*p_vars,
                      struct s_gs_type	*p_type,
                      void		*data) {
        int *p_cat = NULL;

        (void)p_type;
        (void)data;

        p_cat = gs_vars_pop(p_vars, "_category_code", NULL);

        if (!p_cat)
                GS_FAILURE("categoy code not found");

        return *p_cat;
}

static
void
_field_count_push_cb(void		*p_vars,
                       struct s_gs_type	*p_type,
                       void		*data) {
        gs_vars_push(p_vars, p_type, "_field_count", data);
}

static
long
int
_field_count_pop_cb(void		*p_vars,
                      struct s_gs_type	*p_type,
                      void		*data) {
        long int *p_cat = NULL;

        (void)p_type;
        (void)data;

        p_cat = gs_vars_pop(p_vars, "_field_count", NULL);

        if (!p_cat)
                GS_FAILURE("categoy code not found");

        return *p_cat;
}

void
gs_bootstrap_type_bag(struct s_gs_type_bag *p_bag) {
  DEBUG0("###################### BEGIN OF TYPE BOOTSTRAPING");
  gs_bootstrap_incoming_connection(p_bag, NULL);
  DEBUG0("###################### END OF TYPE BOOTSTRAPING");
}

void
gs_bootstrap_incoming_connection(struct s_gs_type_bag	*p_bag,
                                 struct s_gs_connection	*p_cnx) {

        /* basic types */
	struct s_gs_type *signed_char				= NULL;
	struct s_gs_type *unsigned_char				= NULL;
	struct s_gs_type *signed_short_int			= NULL;
	struct s_gs_type *unsigned_short_int			= NULL;
	struct s_gs_type *signed_int				= NULL;
	struct s_gs_type *unsigned_int				= NULL;
	struct s_gs_type *signed_long_int			= NULL;
	struct s_gs_type *unsigned_long_int			= NULL;
	struct s_gs_type *signed_long_long_int			= NULL;
	struct s_gs_type *unsigned_long_long_int		= NULL;
        struct s_gs_type *data_pointer				= NULL;
        struct s_gs_type *function_pointer			= NULL;
        struct s_gs_type *char_array				= NULL;
        struct s_gs_type *string				= NULL;

        /* categories */
        struct s_gs_type *s_gs_cat_elemental			= NULL;		/* Elemental	*/

        struct s_gs_type *s_gs_cat_struct_field			= NULL;		/* Struct	*/
        struct s_gs_type *ref_s_gs_cat_struct_field		= NULL;
        struct s_gs_type *tab_ref_s_gs_cat_struct_field		= NULL;
        struct s_gs_type *ref_tab_ref_s_gs_cat_struct_field	= NULL;
        struct s_gs_type *s_gs_cat_struct			= NULL;

        struct s_gs_type *s_gs_cat_union_field			= NULL;		/* Union	*/
        struct s_gs_type *ref_s_gs_cat_union_field		= NULL;
        struct s_gs_type *tab_ref_s_gs_cat_union_field		= NULL;
        struct s_gs_type *ref_tab_ref_s_gs_cat_union_field	= NULL;
        struct s_gs_type *s_gs_cat_union			= NULL;

        struct s_gs_type *s_gs_cat_ref				= NULL;		/* Ref		*/

        struct s_gs_type *s_gs_cat_array			= NULL;		/* Array	*/

        struct s_gs_type *s_gs_cat_ignored			= NULL;		/* Ignored	*/

        /* type */
	struct s_gs_type *ref_s_gs_cat_elemental		= NULL;
	struct s_gs_type *ref_s_gs_cat_struct			= NULL;
	struct s_gs_type *ref_s_gs_cat_union			= NULL;
	struct s_gs_type *ref_s_gs_cat_ref			= NULL;
	struct s_gs_type *ref_s_gs_cat_array			= NULL;
	struct s_gs_type *ref_s_gs_cat_ignored			= NULL;

        struct s_gs_type *u_gs_category				= NULL;
        struct s_gs_type *s_gs_type				= NULL;

        /* message */
        struct s_gs_type *s_gs_sequence				= NULL;
        struct s_gs_type *s_gs_message				= NULL;

        struct s_gs_type *ref_s_gs_sequence			= NULL;
        struct s_gs_type *ref_s_gs_message			= NULL;


        /*
         * connection boostrap sequence
         */

        signed_char   = gs_type_new_signed_integer_elemental  (p_bag, p_cnx, "signed char",   1);
        unsigned_char = gs_type_new_unsigned_integer_elemental(p_bag, p_cnx, "unsigned char", 1);


        if (p_cnx) {
                struct s_gs_connection_ops 	*cops	= p_cnx->connection_ops;
                unsigned char			 a	= 0;
                unsigned char			 b	= 0;

                if (p_cnx->direction != e_gs_connection_direction_incoming)
                GS_FAILURE("invalid connection");

                cops->read(p_cnx, &b, 1);
                signed_short_int	= gs_type_new_signed_integer_elemental  (p_bag, p_cnx, "signed short int",     	 b);

                cops->read(p_cnx, &b, 1);
                unsigned_short_int	= gs_type_new_unsigned_integer_elemental(p_bag, p_cnx, "unsigned short int",	 b);


                cops->read(p_cnx, &b, 1);
                signed_int		= gs_type_new_signed_integer_elemental  (p_bag, p_cnx, "signed int",		 b);

                cops->read(p_cnx, &b, 1);
                unsigned_int		= gs_type_new_unsigned_integer_elemental(p_bag, p_cnx, "unsigned int",		 b);


                cops->read(p_cnx, &b, 1);
                signed_long_int		= gs_type_new_signed_integer_elemental  (p_bag, p_cnx, "signed long int",	 b);

                cops->read(p_cnx, &b, 1);
                unsigned_long_int	= gs_type_new_unsigned_integer_elemental(p_bag, p_cnx, "unsigned long int",	 b);


                cops->read(p_cnx, &b, 1);
                signed_long_long_int	= gs_type_new_signed_integer_elemental  (p_bag, p_cnx, "signed long long int",	 b);

                cops->read(p_cnx, &b, 1);
                unsigned_long_long_int	= gs_type_new_unsigned_integer_elemental(p_bag, p_cnx, "unsigned long long int", b);

                cops->read(p_cnx, &b, 1);
                cops->read(p_cnx, &a, 1);
                data_pointer		= gs_type_new_ignored(p_bag, p_cnx, "data pointer",	b, a, NULL);

                cops->read(p_cnx, &b, 1);
                cops->read(p_cnx, &a, 1);
                function_pointer	= gs_type_new_ignored(p_bag, p_cnx, "function pointer", b, a, NULL);
        } else {
                signed_short_int	= gs_type_new_signed_integer_elemental  (p_bag, NULL, "signed short int",		sizeof (signed    short      int));
                unsigned_short_int	= gs_type_new_unsigned_integer_elemental(p_bag, NULL, "unsigned short int",		sizeof (unsigned  short      int));

                signed_int		= gs_type_new_signed_integer_elemental  (p_bag, NULL, "signed int",			sizeof (signed               int));
                unsigned_int		= gs_type_new_unsigned_integer_elemental(p_bag, NULL, "unsigned int",			sizeof (unsigned             int));

                signed_long_int		= gs_type_new_signed_integer_elemental  (p_bag, NULL, "signed long int",		sizeof (signed    long       int));
                unsigned_long_int	= gs_type_new_unsigned_integer_elemental(p_bag, NULL, "unsigned long int",		sizeof (unsigned  long       int));

                signed_long_long_int	= gs_type_new_signed_integer_elemental  (p_bag, NULL, "signed long long int",		sizeof (signed    long long  int));
                unsigned_long_long_int	= gs_type_new_unsigned_integer_elemental(p_bag, NULL, "unsigned long long int",	sizeof (unsigned  long long  int));

                data_pointer		= gs_type_new_ignored(p_bag, NULL, "data pointer",	sizeof (void *),	  sizeof (void *),	    NULL);
                function_pointer	= gs_type_new_ignored(p_bag, NULL, "function pointer", sizeof (void (*) (void)), sizeof (void (*) (void)), NULL);
        }

        char_array = gs_type_new_array_with_callback(p_bag, p_cnx, "_char_array", -1, signed_char, _strlen_cb, NULL);
        string	   = gs_type_new_ref(p_bag, p_cnx, "_string", char_array);

        /* Elemental cat */
        s_gs_cat_elemental = gs_type_new_struct(p_bag, p_cnx, "_s_gs_cat_elemental");
        gs_type_struct_append_field(s_gs_cat_elemental, "encoding", signed_int);

        /* Struct cat */
        s_gs_cat_struct_field = gs_type_new_struct(p_bag, p_cnx, "_s_gs_cat_struct_field");
        gs_type_struct_append_field(s_gs_cat_struct_field, "name", 	      string);
        gs_type_struct_append_field(s_gs_cat_struct_field, "offset",	      signed_long_int);
        gs_type_struct_append_field(s_gs_cat_struct_field, "code",	      signed_int);
        gs_type_struct_append_field(s_gs_cat_struct_field, "before_callback", function_pointer);
        gs_type_struct_append_field(s_gs_cat_struct_field, "after_callback",  function_pointer);

        ref_s_gs_cat_struct_field 	  = gs_type_new_ref                (p_bag, p_cnx, "_ps_gs_cat_struct_field",   s_gs_cat_struct_field);
        tab_ref_s_gs_cat_struct_field	  = gs_type_new_array_with_callback(p_bag, p_cnx, "_aps_gs_cat_struct_field", -1, ref_s_gs_cat_struct_field, _field_count_pop_cb, NULL);
        ref_tab_ref_s_gs_cat_struct_field = gs_type_new_ref                (p_bag, p_cnx, "_paps_gs_cat_struct_field", tab_ref_s_gs_cat_struct_field);

        s_gs_cat_struct = gs_type_new_struct(p_bag, p_cnx, "_s_gs_cat_struct");
        gs_type_struct_append_field_with_callback(s_gs_cat_struct, "nb_fields", signed_int, _field_count_push_cb, NULL);
        gs_type_struct_append_field(s_gs_cat_struct, "field_array", ref_tab_ref_s_gs_cat_struct_field);

        /* Union cat */
        s_gs_cat_union_field = gs_type_new_struct(p_bag, p_cnx, "_s_gs_cat_union_field");
        gs_type_struct_append_field(s_gs_cat_union_field, "name", 	   string);
        gs_type_struct_append_field(s_gs_cat_union_field, "code",		   signed_int);
        gs_type_struct_append_field(s_gs_cat_union_field, "before_callback", function_pointer);
        gs_type_struct_append_field(s_gs_cat_union_field, "after_callback",  function_pointer);

        ref_s_gs_cat_union_field 	 = gs_type_new_ref                (p_bag, p_cnx, "_ps_gs_cat_union_field",   s_gs_cat_union_field);
        tab_ref_s_gs_cat_union_field	 = gs_type_new_array_with_callback(p_bag, p_cnx, "_aps_gs_cat_union_field", -1, ref_s_gs_cat_union_field, _field_count_pop_cb, NULL);
        ref_tab_ref_s_gs_cat_union_field = gs_type_new_ref                (p_bag, p_cnx, "_paps_gs_cat_union_field", tab_ref_s_gs_cat_union_field);

        s_gs_cat_union = gs_type_new_struct(p_bag, p_cnx, "_s_gs_cat_union");
        gs_type_struct_append_field_with_callback(s_gs_cat_union, "nb_fields", signed_int, _field_count_push_cb, NULL);
        gs_type_struct_append_field(s_gs_cat_union, "field_array", ref_tab_ref_s_gs_cat_union_field);
        gs_type_struct_append_field(s_gs_cat_union, "callback",	 function_pointer);

        /* Ref cat */
        s_gs_cat_ref = gs_type_new_struct(p_bag, p_cnx, "_s_gs_cat_ref");
        gs_type_struct_append_field(s_gs_cat_ref, "code",	    signed_int);
        gs_type_struct_append_field(s_gs_cat_ref, "callback", function_pointer);

        /* Array cat */
        s_gs_cat_array = gs_type_new_struct(p_bag, p_cnx, "_s_gs_cat_array");
        gs_type_struct_append_field(s_gs_cat_array, "code",	   signed_int);
        gs_type_struct_append_field(s_gs_cat_array, "element_count", signed_long_int);
        gs_type_struct_append_field(s_gs_cat_array, "callback",	   function_pointer);

        /* Ignored cat */
        s_gs_cat_ignored = gs_type_new_struct(p_bag, p_cnx, "_s_gs_cat_ignored");
        gs_type_struct_append_field(s_gs_cat_ignored, "default_value", data_pointer);

        /* refs to categories */
        ref_s_gs_cat_elemental	= gs_type_new_ref(p_bag, p_cnx, "_ref_s_gs_cat_elemental", s_gs_cat_elemental);
        ref_s_gs_cat_struct     = gs_type_new_ref(p_bag, p_cnx, "_ref_s_gs_cat_struct",	   s_gs_cat_struct   );
        ref_s_gs_cat_union      = gs_type_new_ref(p_bag, p_cnx, "_ref_s_gs_cat_union",	   s_gs_cat_union    );
        ref_s_gs_cat_ref        = gs_type_new_ref(p_bag, p_cnx, "_ref_s_gs_cat_ref",	   s_gs_cat_ref      );
        ref_s_gs_cat_array      = gs_type_new_ref(p_bag, p_cnx, "_ref_s_gs_cat_array",	   s_gs_cat_array    );
        ref_s_gs_cat_ignored	= gs_type_new_ref(p_bag, p_cnx, "_ref_s_gs_cat_ignored",   s_gs_cat_ignored  );

        /* union of categories */
        u_gs_category = gs_type_new_union(p_bag, p_cnx, "_u_gs_category", _category_code_pop_cb);
        gs_type_union_append_field(u_gs_category, "undefined_data", data_pointer);
        gs_type_union_append_field(u_gs_category, "elemental_data", ref_s_gs_cat_elemental);
        gs_type_union_append_field(u_gs_category, "struct_data",    ref_s_gs_cat_struct);
        gs_type_union_append_field(u_gs_category, "union_data",     ref_s_gs_cat_union);
        gs_type_union_append_field(u_gs_category, "ref_data",       ref_s_gs_cat_ref);
        gs_type_union_append_field(u_gs_category, "array_data",     ref_s_gs_cat_array);
        gs_type_union_append_field(u_gs_category, "ignored_data",   ref_s_gs_cat_ignored);

        /* type */
        s_gs_type = gs_type_new_struct(p_bag, p_cnx, "_s_gs_type");
        gs_type_struct_append_field              (s_gs_type, "code",		signed_int);
        gs_type_struct_append_field              (s_gs_type, "name",		string);
        gs_type_struct_append_field              (s_gs_type, "size",		signed_long_int);
        gs_type_struct_append_field              (s_gs_type, "alignment",		signed_long_int);
        gs_type_struct_append_field              (s_gs_type, "aligned_size",	signed_long_int);
        gs_type_struct_append_field_with_callback(s_gs_type, "category_code",	signed_int, _category_code_push_cb, NULL);
        gs_type_struct_append_field              (s_gs_type, "category",		u_gs_category);
        gs_type_struct_append_field              (s_gs_type, "before_callback",	function_pointer);
        gs_type_struct_append_field              (s_gs_type, "after_callback",	function_pointer);

        /* message */
        s_gs_sequence = gs_type_new_struct(p_bag, p_cnx, "_s_gs_sequence");
        s_gs_message  = gs_type_new_struct(p_bag, p_cnx, "_s_gs_message");

        ref_s_gs_sequence = gs_type_new_ref(p_bag, p_cnx, "_ref_s_gs_sequence", s_gs_sequence);
        ref_s_gs_message  = gs_type_new_ref(p_bag, p_cnx, "_ref_s_gs_message",  s_gs_message);

        gs_type_struct_append_field(s_gs_sequence, "message", ref_s_gs_message);
        gs_type_struct_append_field(s_gs_sequence, "code",    signed_int);
        gs_type_struct_append_field(s_gs_sequence, "next",    ref_s_gs_sequence);

        gs_type_struct_append_field(s_gs_message, "code",  signed_int);
        gs_type_struct_append_field(s_gs_message, "name",  string);
        gs_type_struct_append_field(s_gs_message, "first", ref_s_gs_sequence);
        gs_type_struct_append_field(s_gs_message, "next",	 ref_s_gs_sequence);
}

void
gs_bootstrap_outgoing_connection(struct s_gs_type_bag	*p_bag,
                                 struct s_gs_connection	*p_cnx) {
        struct s_gs_type_bag_ops	*bops	= p_bag->bag_ops;
        struct s_gs_connection_ops 	*cops	= p_cnx->connection_ops;
        unsigned char			 b	= 0;

        if (p_cnx->direction != e_gs_connection_direction_outgoing)
                GS_FAILURE("invalid connection");

        bops->mark_type(p_bag, p_cnx, "signed char");
        bops->mark_type(p_bag, p_cnx, "unsigned char");

        b = sizeof(signed short int);
        cops->write(p_cnx, &b, 1);
        bops->mark_type(p_bag, p_cnx, "signed short int");

        b = sizeof(unsigned short int);
        cops->write(p_cnx, &b, 1);
        bops->mark_type(p_bag, p_cnx, "unsigned short int");

        b = sizeof(signed int);
        cops->write(p_cnx, &b, 1);
        bops->mark_type(p_bag, p_cnx, "signed int");

        b = sizeof(unsigned int);
        cops->write(p_cnx, &b, 1);
        bops->mark_type(p_bag, p_cnx, "unsigned in");

        b = sizeof(signed long int);
        cops->write(p_cnx, &b, 1);
        bops->mark_type(p_bag, p_cnx, "signed long int");

        b = sizeof(unsigned long int);
        cops->write(p_cnx, &b, 1);
        bops->mark_type(p_bag, p_cnx, "unsigned long int");

        b = sizeof(signed long long int);
        cops->write(p_cnx, &b, 1);
        bops->mark_type(p_bag, p_cnx, "signed long long int");

        b = sizeof(unsigned long long int);
        cops->write(p_cnx, &b, 1);
        bops->mark_type(p_bag, p_cnx, "unsigned long long int");

        b = sizeof(void *);
        cops->write(p_cnx, &b, 1); /* size 	*/
        cops->write(p_cnx, &b, 1); /* alignment */
        bops->mark_type(p_bag, p_cnx, "data pointer");

        b = sizeof(void (*) (void));
        cops->write(p_cnx, &b, 1); /* size 	*/
        cops->write(p_cnx, &b, 1); /* alignment */
        bops->mark_type(p_bag, p_cnx, "function pointer");

        bops->mark_type(p_bag, p_cnx, "_char_array");
        bops->mark_type(p_bag, p_cnx, "_string");

        bops->mark_type(p_bag, p_cnx, "_s_gs_cat_elemental");

        bops->mark_type(p_bag, p_cnx, "_s_gs_cat_struct_field");
        bops->mark_type(p_bag, p_cnx, "_ps_gs_cat_struct_field");
        bops->mark_type(p_bag, p_cnx, "_aps_gs_cat_struct_field");
        bops->mark_type(p_bag, p_cnx, "_paps_gs_cat_struct_field");
        bops->mark_type(p_bag, p_cnx, "_s_gs_cat_struct");

        bops->mark_type(p_bag, p_cnx, "_s_gs_cat_union_field");
        bops->mark_type(p_bag, p_cnx, "_ps_gs_cat_union_field");
        bops->mark_type(p_bag, p_cnx, "_aps_gs_cat_union_field");
        bops->mark_type(p_bag, p_cnx, "_paps_gs_cat_union_field");
        bops->mark_type(p_bag, p_cnx, "_s_gs_cat_union");

        bops->mark_type(p_bag, p_cnx, "_s_gs_cat_ref");

        bops->mark_type(p_bag, p_cnx, "_s_gs_cat_array");

        bops->mark_type(p_bag, p_cnx, "_s_gs_cat_ignored");

        bops->mark_type(p_bag, p_cnx, "_ref_s_gs_cat_elemental");
        bops->mark_type(p_bag, p_cnx, "_ref_s_gs_cat_struct");
        bops->mark_type(p_bag, p_cnx, "_ref_s_gs_cat_union");
        bops->mark_type(p_bag, p_cnx, "_ref_s_gs_cat_ref");
        bops->mark_type(p_bag, p_cnx, "_ref_s_gs_cat_array");
        bops->mark_type(p_bag, p_cnx, "_ref_s_gs_cat_ignored");

        bops->mark_type(p_bag, p_cnx, "_u_gs_category");
        bops->mark_type(p_bag, p_cnx, "_s_gs_type");

        bops->mark_type(p_bag, p_cnx, "_s_gs_sequence");
        bops->mark_type(p_bag, p_cnx, "_s_gs_message" );

        bops->mark_type(p_bag, p_cnx, "_ref_s_gs_sequence");
        bops->mark_type(p_bag, p_cnx, "_ref_s_gs_message");
}


/*......................................................................
 * Ignored
 */
static
struct s_gs_cat_ignored *
_gs_ignored_cat_alloc(void) {
        struct s_gs_cat_ignored	*p_ignored	= NULL;

        p_ignored = malloc(sizeof (struct s_gs_cat_ignored));

        return p_ignored;
}

static
struct s_gs_cat_ignored *
_gs_ignored_cat_new(void) {
        struct s_gs_cat_ignored	*p_ignored	= NULL;

        p_ignored			= _gs_ignored_cat_alloc();
        p_ignored->default_value	= NULL;

        return p_ignored;
}


struct s_gs_type *
gs_type_new_ignored_with_callback(struct s_gs_type_bag	*p_bag,
                                  struct s_gs_connection	*p_connection,
                                  const char                          *name,
                                  long int                       size,
                                  long int                       alignment,
                                  void				*default_value,
                                  void (*callback)(void		*vars,
                                                   struct s_gs_type	*p_type,
                                                   void		*data)) {

        struct s_gs_type 		*p_type		= NULL;
        struct s_gs_cat_ignored		*p_ignored	= NULL;

        p_ignored	= _gs_ignored_cat_new();
        p_type		= _gs_type_new(p_bag, p_connection, name);

        p_type->size		= size > 0?size:0;
        p_type->alignment	= alignment;

        if (size > 0) {
                long int aligned_size	= aligned(size, alignment);

                p_type->aligned_size	= aligned_size;
        } else {
                p_type->aligned_size	= 0;
        }

        if (default_value && p_type->size) {
                p_ignored->default_value = gs_memdup(default_value, (size_t)size);
        }

        p_type->category_code           = e_gs_type_cat_ignored;
        p_type->category.ignored_data	= p_ignored;

        p_type->before_callback		= callback;

        return p_type;
}

struct s_gs_type *
gs_type_new_ignored(struct s_gs_type_bag	*p_bag,
                    struct s_gs_connection	*p_connection,
                    const char                        *name,
                    long int                     size,
                    long int                     alignment,
                    void			*default_value) {
        return gs_type_new_ignored_with_callback(p_bag, p_connection, name, size, alignment, default_value, NULL);
}
