/* gs_message.c */

#include "DataDesc/gs_private.h"

/*
 * Sequences
 */
static
struct s_gs_sequence *
_gs_sequence_alloc(void) {
        struct s_gs_sequence 		*p_sequence		= NULL;
        p_sequence = malloc(sizeof(struct s_gs_sequence));
        return p_sequence;
}

static
struct s_gs_sequence *
_gs_sequence_new(void) {
        struct s_gs_sequence 		*p_sequence		= NULL;

        p_sequence = _gs_sequence_alloc();

        p_sequence->message = NULL;
        p_sequence->code    =   -1;
        p_sequence->next    = NULL;

        return p_sequence;
}

/*
 * Messages
 */
static
struct s_gs_message *
_gs_message_alloc(void) {
        struct s_gs_message *p_message = NULL;

        p_message = malloc(sizeof(struct s_gs_message));

        return p_message;
}

struct s_gs_message *
gs_message_new(struct s_gs_type_bag	*p_bag,
               struct s_gs_connection	*p_connection,
               const char		*name) {

        struct s_gs_type_bag_ops	*bag_ops	= p_bag->bag_ops;
        struct s_gs_message		*p_message	= NULL;

        p_message        = _gs_message_alloc();

        p_message->code  = -1;
        p_message->name  = strdup(name);

        p_message->first = NULL;
        p_message->last  = NULL;

        bag_ops->store_message(p_bag, p_connection, p_message);

        return p_message;
}


void
gs_message_append_new_sequence(struct s_gs_message	*p_message,
                               struct s_gs_type		*p_type) {

        struct s_gs_sequence *p_sequence = NULL;

        p_sequence = _gs_sequence_new();

        p_sequence->message = p_message;
        p_sequence->code    = p_type->code;
        p_sequence->next    = NULL;

        if (p_message->first) {
                p_message->last ->next = p_sequence;
        } else {
                p_message->first       = p_sequence;
        }

        p_message->last = p_sequence;
}


/*
 * Send/receive
 */
static
struct s_gs_message_instance *
_gs_message_instance_alloc(void) {
        struct s_gs_message_instance *p_message_instance = NULL;
        p_message_instance = malloc(sizeof (struct s_gs_message_instance));
        return p_message_instance;
}

static
struct s_gs_message_instance *
_gs_message_instance_new(void) {
        struct s_gs_message_instance *p_message_instance = NULL;

        p_message_instance = _gs_message_instance_alloc();

        p_message_instance->p_message		= NULL;
        p_message_instance->current		= NULL;
        p_message_instance->p_bag		= NULL;
        p_message_instance->p_connection	= NULL;

        return p_message_instance;
}

/* Send */
static
void
_gs_message_send_type(struct s_gs_type_bag	 *p_bag,
                      struct s_gs_connection	 *p_cnx,
                      void			 *vars,
                      gras_dict_t		**p_refs,
                      struct s_gs_type		 *p_type,
                      void			 *data);

static
struct s_gs_type *
_sg_message_send_get_type_by_code(struct s_gs_type_bag	 *p_bag,
                                  struct s_gs_connection *p_cnx,
                                  void			 *vars,
                                  int			  code) {
        struct s_gs_type_bag_ops	*bops	= p_bag->bag_ops;
        struct s_gs_type		*p_type = NULL;

        p_type = bops->get_type_by_code(p_bag, NULL, code);

        if (!bops->check_type_mark(p_bag, p_cnx, p_type->name)) {
                struct s_gs_type	*p_type_type = NULL;
                gras_dict_t		*_refs       = NULL;

                p_type_type = bops->get_type_by_name(p_bag, NULL, "_s_gs_type");
                if (!p_type_type)
                        GS_FAILURE("_s_gs_type default type missing");

                bops->mark_type(p_bag, p_cnx, p_type->name);

                fprintf(stderr, "sending type with code = %d\n", code);
                gs_vars_enter(vars);
                _gs_message_send_type(p_bag, p_cnx, vars, &_refs, p_type_type, p_type);
                gs_vars_leave(vars);
                fprintf(stderr, "type with code = %d, name = %s sent\n", code, p_type->name);
        } else {
                fprintf(stderr, "no need to send type with code = %d, name = %s\n", code, p_type->name);
        }

        return p_type;
}

static
void
_gs_message_send_type(struct s_gs_type_bag	 *p_bag,
                      struct s_gs_connection	 *p_cnx,
                      void			 *vars,
                      gras_dict_t		**p_refs,
                      struct s_gs_type		 *p_type,
                      void			 *data) {

        struct s_gs_type_bag_ops	*bops = p_bag->bag_ops;
        struct s_gs_connection_ops	*cops = p_cnx->connection_ops;

	if (!*p_refs)
	  gras_dict_new(p_refs);

        if (p_type->category_code > 1)
                fprintf(stderr, "\n");
        fprintf(stderr, "sending a %s\n", p_type->name);

        switch (p_type->category_code) {

        case e_gs_type_cat_elemental:
                {
                        if (p_type->before_callback) {
                                p_type->before_callback(vars, p_type, data);
                        }

                        cops->write (p_cnx, data, p_type->size);
                }
                break;

        case e_gs_type_cat_struct   :
                {
                        struct s_gs_cat_struct *p_struct  = NULL;

                        if (p_type->before_callback) {
                                p_type->before_callback(vars, p_type, data);
                        }

                        p_struct = p_type->category.struct_data;

                        {
                                int nb_fields = 0;
                                int i         = 0;

                                nb_fields = p_struct->nb_fields;

                                while (i < nb_fields) {
                                        struct s_gs_cat_struct_field	*p_field	= NULL;
                                        struct s_gs_type		*p_field_type	= NULL;
                                        void				*ref		= NULL;

                                        p_field		= p_struct->field_array[i];
                                        p_field_type	= _sg_message_send_get_type_by_code(p_bag, p_cnx, vars, p_field->code);
                                        ref		= ((char *)data) + p_field->offset;

                                        if (p_field->before_callback) {
                                                p_field->before_callback(vars, p_field_type, ref);
                                        }

                                        _gs_message_send_type(p_bag,
                                                              p_cnx,
                                                              vars,
                                                              p_refs,
                                                              p_field_type,
                                                              ref);

                                        if (p_field->after_callback) {
                                                p_field->after_callback(vars, p_field_type, ref);
                                        }

                                        i++;
                                }
                        }

                        if (p_type->after_callback) {
                                p_type->after_callback(vars, p_type, data);
                        }
                }
                break;

        case e_gs_type_cat_union   :
                {
                        struct s_gs_cat_union *p_union  = NULL;

                        p_union = p_type->category.union_data;

                        {
                                int				 field_num	= 0;
                                struct s_gs_cat_union_field	*p_field	= NULL;
                                struct s_gs_type		*p_field_type	= NULL;

                                field_num = p_union->callback(vars, p_type, data);

                                if ((field_num < 0) || (field_num >= p_union->nb_fields))
                                        GS_FAILURE("invalid field index");

                                _gs_message_send_type(p_bag,
                                                      p_cnx,
                                                      vars,
                                                      p_refs,
                                                      bops->get_type_by_name(p_bag, NULL, "signed int"),
                                                      &field_num);

                                p_field 	= p_union->field_array[field_num];
                                p_field_type	= _sg_message_send_get_type_by_code(p_bag, p_cnx, vars, p_field->code);

                                if (p_field->before_callback) {
                                        p_field->before_callback(vars, p_field_type, data);
                                }

                                _gs_message_send_type(p_bag,
                                                      p_cnx,
                                                      vars,
                                                      p_refs,
                                                      p_field_type,
                                                      data);

                                if (p_field->after_callback) {
                                        p_field->after_callback(vars, p_field_type, data);
                                }
                        }

                        if (p_type->after_callback) {
                                p_type->after_callback(vars, p_type, data);
                        }
                }
                break;

        case e_gs_type_cat_ref      :
                {
                        void	 		*dummy		= NULL;
                        void			**p_ref		= (void **)data;
                        struct s_gs_cat_ref	*p_ref_data	= NULL;
                        int       		 code		= 0;

                        p_ref_data = p_type->category.ref_data;

                        if (p_ref_data->callback) {
                                code = p_ref_data->callback(vars, p_type, data);
                        }

                        if (p_ref_data->code >= 0) {
                                code = p_ref_data->code;
                        } else {
                                _gs_message_send_type(p_bag,
                                                      p_cnx,
                                                      vars,
                                                      p_refs,
                                                      bops->get_type_by_name(p_bag, NULL, "signed int"),
                                                      &code);
                        }

                        cops->write (p_cnx, data, p_type->size);

                        if (*p_ref && !(gras_dict_retrieve_ext(*p_refs, (char *)p_ref, sizeof(void *), &dummy), dummy)) {
                                struct s_gs_type	*p_ref_type	= NULL;

                                fprintf(stderr, "sending data referenced at %p\n", *p_ref);
                                gras_dict_insert_ext(*p_refs, (char *)p_ref, sizeof(void *), p_ref, NULL);

                                p_ref_type = _sg_message_send_get_type_by_code(p_bag, p_cnx, vars, code);

                                _gs_message_send_type(p_bag,
                                                      p_cnx,
                                                      vars,
                                                      p_refs,
                                                      p_ref_type,
                                                      *p_ref);

                        } else {
                                /**/
                                fprintf(stderr, "not sending data referenced at %p\n", *p_ref);
                        }

                        if (p_type->after_callback) {
                                p_type->after_callback(vars, p_type, data);
                        }
                }
                break;

        case e_gs_type_cat_array    :
                {
                        struct s_gs_cat_array *p_array        = NULL;
                        long int               count          =    0;
                        struct s_gs_type      *p_element_type = NULL;
                        long int               element_size   =    0;
                        char                  *ptr            = data;

                        p_array = p_type->category.array_data;
                        count   = p_array->element_count;

                        if (count < 0) {
                                count = p_array->callback(vars, p_type, data);
                                if (count < 0)
                                        GS_FAILURE("invalid array element count");
                        } else if (p_array->callback) {
                                (void)p_array->callback(vars, p_type, data);
                        }

                        _gs_message_send_type(p_bag,
                                              p_cnx,
                                              vars,
                                              p_refs,
                                              bops->get_type_by_name(p_bag, NULL, "signed long int"),
                                              &count);

                        p_element_type	= _sg_message_send_get_type_by_code(p_bag, p_cnx, vars, p_array->code);
                        element_size	= p_element_type->aligned_size;

                        if (count == 0)
                                goto empty_array;

                        while (count --) {
                                _gs_message_send_type(p_bag,
                                                      p_cnx,
                                                      vars,
                                                      p_refs,
                                                      p_element_type,
                                                      ptr);

                                ptr += element_size;
                        }

                empty_array:
                        if (p_type->after_callback) {
                                p_type->after_callback(vars, p_type, data);
                        }
                }
                break;


       case e_gs_type_cat_ignored    :
                {
                        if (p_type->before_callback) {
                                p_type->before_callback(vars, p_type, data);
                        }
                }
                break;

         default:
                GS_FAILURE("invalid type");
        }
}

struct s_gs_message_instance *
gs_message_init_send_by_ref(struct s_gs_type_bag	*p_bag,
                            struct s_gs_connection	*p_cnx,
                            struct s_gs_message		*p_message) {
        struct s_gs_type_bag_ops	*bops			= p_bag->bag_ops;
        struct s_gs_message_instance 	*p_message_instance	= NULL;
        void				*vars			= NULL;

        vars = gs_vars_alloc();

        if (!bops->check_message_mark(p_bag, p_cnx, p_message->name)) {
                struct s_gs_type	*p_message_type = NULL;
                gras_dict_t		*_refs		= NULL;
                struct s_gs_type 	*signed_int	= NULL;

                signed_int = bops->get_type_by_name(p_bag, NULL, "signed int");
                p_message_type = bops->get_type_by_name(p_bag, NULL, "_s_gs_message");

                if (!p_message_type)
                        GS_FAILURE("_s_gs_message default type missing");

                bops->mark_message(p_bag, p_cnx, p_message->name);

                _gs_message_send_type(p_bag, p_cnx, vars, &_refs, signed_int,     &p_message->code);
                _gs_message_send_type(p_bag, p_cnx, vars, &_refs, p_message_type,  p_message);
        } else {
                gras_dict_t		*_refs		= NULL;
                struct s_gs_type 	*signed_int	= NULL;

                _gs_message_send_type(p_bag, p_cnx, vars, &_refs, signed_int, &p_message->code);
        }

        gs_vars_free(vars);

        p_message_instance	= _gs_message_instance_new();

        p_message_instance->p_message		= p_message;
        p_message_instance->current		= p_message->first;
        p_message_instance->p_bag		= p_bag;
        p_message_instance->p_connection	= p_cnx;


        return p_message_instance;
}

struct s_gs_message_instance *
gs_message_init_send_by_name(struct s_gs_type_bag	*p_bag,
                             struct s_gs_connection	*p_connection,
                             const char			*name) {
        struct s_gs_type_bag_ops	*bag_ops		= p_bag->bag_ops;
        struct s_gs_message		*p_message		= NULL;
        struct s_gs_message_instance	*p_message_instance	= NULL;

        p_message		= bag_ops->get_message_by_name(p_bag, NULL, name);
        p_message_instance	= gs_message_init_send_by_ref(p_bag, p_connection, p_message);

        return p_message_instance;
}

struct s_gs_message_instance *
gs_message_init_send_by_code(struct s_gs_type_bag	*p_bag,
                             struct s_gs_connection	*p_connection,
                             int			code) {
        struct s_gs_type_bag_ops	*bag_ops		= p_bag->bag_ops;
        struct s_gs_message		*p_message		= NULL;
        struct s_gs_message_instance	*p_message_instance	= NULL;

        p_message 		= bag_ops->get_message_by_code(p_bag, NULL, code);
        p_message_instance	= gs_message_init_send_by_ref(p_bag, p_connection, p_message);

        return p_message_instance;
}

void
gs_message_send_next_sequence_ext(void                         *vars,
                                  struct s_gs_message_instance *p_message_instance,
                                  void                         *data) {
        struct s_gs_type_bag		*p_bag		= p_message_instance->p_bag;
        struct s_gs_connection		*p_cnx		= p_message_instance->p_connection;
        struct s_gs_sequence		*p_sequence	= NULL;
        struct s_gs_type		*p_type		= NULL;
        gras_dict_t			*refs		= NULL;

        p_sequence = p_message_instance->current;
        if (!p_sequence)
                GS_FAILURE("message type has no more sequences");

        p_type = _sg_message_send_get_type_by_code(p_bag, p_cnx, vars, p_sequence->code);
        _gs_message_send_type(p_message_instance->p_bag,
                              p_message_instance->p_connection,
                              vars,
                              &refs,
                              p_type,
                              data);
        gras_dict_free(&refs);
        p_message_instance->current = p_sequence->next;
}

void
gs_message_send_next_sequence(struct s_gs_message_instance *p_message_instance,
                              void                         *data) {
        void *vars = NULL;

        vars = gs_vars_alloc();
        gs_message_send_next_sequence_ext(vars, p_message_instance, data);
        gs_vars_free(vars);
}


/* recv */


/*
 * Note: here we suppose that the remote NULL is a sequence of 'length' bytes set to 0.
 */
static
int
_gs_is_null(void	**p_ptr,
            long int	  length) {
        int i;

        for (i = 0; i < length; i++) {
                if (((unsigned char *)p_ptr)[i]) {
                        return 0;
                }
        }

        return 1;
}


static
void
_gs_alloc_ref(gras_dict_t	**p_refs,
              long int		  size,
              void		**p_old_ref,
              long int		  length,
              void		**p_new_ref) {
        void *new_ref = NULL;

        new_ref	= malloc((size_t)size);
        *p_new_ref	= new_ref;

        if (p_old_ref && !_gs_is_null(p_old_ref, length)) {
                p_old_ref	= gs_memdup(p_old_ref, (size_t)length);
                p_new_ref	= gs_memdup(&new_ref, sizeof(void *));

                gras_dict_insert_ext(*p_refs,(char *) p_old_ref, length, p_new_ref, NULL);
        }
}

static
void
_gs_convert_elemental(void 		*local,
                      struct s_gs_type  *p_local_type,
                      void		*remote,
                      struct s_gs_type  *p_remote_type) {
        struct s_gs_cat_elemental *p_local_elemental  = p_local_type->category.elemental_data;
        struct s_gs_cat_elemental *p_remote_elemental = p_remote_type->category.elemental_data;

        (void)local;
        (void)remote;

        if (p_local_elemental->encoding == p_remote_elemental->encoding) {
                if (p_local_type->size != p_remote_type->size) {
                        GS_FAILURE("size conversion unimplemented");
                }
        } else {
                GS_FAILURE("encoding conversion unimplemented");
        }
}

static
void
_gs_message_receive_type(struct s_gs_type_bag	 *p_bag,
                         struct s_gs_connection	 *p_cnx,
                         gras_dict_t            **p_refs,
                         struct s_gs_type        *p_type,
                         void                   **p_old_data,
                         long int		  old_data_length,
                         void                   **p_new_data);

static
struct s_gs_type *
_sg_message_receive_get_type_by_code(struct s_gs_type_bag	*p_bag,
                                     struct s_gs_connection	*p_cnx,
                                     int			 code) {
        struct s_gs_type_bag_ops	*bops	= p_bag->bag_ops;
        struct s_gs_type		*p_type = NULL;

        p_type = bops->get_type_by_code(p_bag, p_cnx, code);

        if (!p_type) {
                struct s_gs_type	*p_type_type = NULL;
                gras_dict_t		*_refs       = NULL;

                p_type_type = bops->get_type_by_name(p_bag, p_cnx, "_s_gs_type");
                if (!p_type_type)
                        GS_FAILURE("_s_gs_type default type missing");

                fprintf(stderr, "need type with code = %d\n", code);
                _gs_message_receive_type(p_bag, p_cnx, &_refs, p_type_type, NULL, 0, (void **)&p_type);
                fprintf(stderr, "got type with code = %d, name = %s\n", code, p_type->name);

                bops->store_incoming_type(p_bag, p_cnx, p_type);
        } else {
                fprintf(stderr, "already seen type with code = %d, name = %s\n", code, p_type->name);
        }
        

        return p_type;
}

static
void
_gs_message_receive_type(struct s_gs_type_bag	 *p_bag,
                         struct s_gs_connection	 *p_cnx,
                         gras_dict_t            **p_refs,
                         struct s_gs_type        *p_remote_type,
                         void                   **p_old_ptr,
                         long int		  old_ptr_length,
                         void                   **p_new_ptr) {
        struct s_gs_type_bag_ops	*bops		= p_bag->bag_ops;
        struct s_gs_connection_ops	*cops		= p_cnx->connection_ops;
        struct s_gs_type        	*p_local_type	= NULL;
        void				*new_data	= *p_new_ptr;

	if (!*p_refs)
	  gras_dict_new(p_refs);

        p_local_type = bops->get_type_by_name(p_bag, NULL, p_remote_type->name);
        if (p_local_type->category_code > 1)
                fprintf(stderr, "\n");
        fprintf(stderr, "receiving a %s\n", p_local_type->name);

        switch (p_remote_type->category_code) {

        case e_gs_type_cat_elemental:
                {
                        if (!new_data) {
                                _gs_alloc_ref(p_refs, p_local_type->size, p_old_ptr, old_ptr_length, p_new_ptr);
                                new_data = *p_new_ptr;
                        }

                        if (p_remote_type->size <= p_local_type->size) {
                                cops->read (p_cnx, new_data, p_remote_type->size);
                                _gs_convert_elemental(new_data, p_local_type, new_data, p_remote_type);
                        } else {
                                void *ptr = NULL;

                                ptr = malloc((size_t)p_remote_type->size);
                                cops->read (p_cnx, ptr, p_remote_type->size);
                                _gs_convert_elemental(new_data, p_local_type, ptr, p_remote_type);
                                free(ptr);
                        }
                }
                break;

        case e_gs_type_cat_struct   :
                {
                        struct s_gs_cat_struct *p_struct  = NULL;

                        p_struct = p_remote_type->category.struct_data;

                        if (!new_data) {
                                _gs_alloc_ref(p_refs, p_local_type->size, p_old_ptr, old_ptr_length, p_new_ptr);
                                new_data = *p_new_ptr;
                        }

                        {
                                int nb_fields = 0;
                                int i         = 0;

                                nb_fields = p_struct->nb_fields;

                                while (i < nb_fields) {
                                        struct s_gs_cat_struct_field	*p_field	= NULL;
                                        struct s_gs_type		*p_field_type	= NULL;
                                        void				*ref		= NULL;

                                        p_field		= p_struct->field_array[i];
                                        p_field_type	= _sg_message_receive_get_type_by_code(p_bag, p_cnx, p_field->code);
                                        ref		= ((char *)new_data) + p_field->offset;

                                        _gs_message_receive_type(p_bag,
                                                                 p_cnx,
                                                                 p_refs,
                                                                 p_field_type,
                                                                 NULL,
                                                                 0,
                                                                 &ref);
                                        i++;
                                }
                        }
                }
                break;

        case e_gs_type_cat_union   :
                {
                        struct s_gs_cat_union *p_union  = NULL;

                        p_union = p_remote_type->category.union_data;

                        if (!new_data) {
                                _gs_alloc_ref(p_refs, p_local_type->size, p_old_ptr, old_ptr_length, p_new_ptr);
                                new_data = *p_new_ptr;
                        }

                        {
                                int				 field_num	= 0;
                                struct s_gs_cat_union_field	*p_field	= NULL;
                                struct s_gs_type		*p_field_type	= NULL;

                                {
                                        int *p_field_num = &field_num;

                                        _gs_message_receive_type(p_bag,
                                                                 p_cnx,
                                                                 p_refs,
                                                                 bops->get_type_by_name(p_bag, p_cnx, "signed int"),
                                                                 NULL,
                                                                 0,
                                                                 (void**)&p_field_num);
                                }

                                p_field		= p_union->field_array[field_num];
                                p_field_type	= _sg_message_receive_get_type_by_code(p_bag, p_cnx, p_field->code);

                                _gs_message_receive_type(p_bag,
                                                         p_cnx,
                                                         p_refs,
                                                         p_field_type,
                                                         NULL,
                                                         0,
                                                         &new_data);
                        }
                }
                break;

        case e_gs_type_cat_ref      :
                {
                        void			**p_old_ref	= NULL;
                        void			**p_new_ref	= NULL;
                        struct s_gs_cat_ref	 *p_ref_data	= NULL;
                        int       		  code		= 0;

                        p_ref_data	= p_remote_type->category.ref_data;
                        code		= p_ref_data->code;

                        if (code < 0) {
                                int *p_code = &code;

                                _gs_message_receive_type(p_bag,
                                                         p_cnx,
                                                         p_refs,
                                                         bops->get_type_by_name(p_bag, p_cnx, "signed int"),
                                                         NULL,
                                                         0,
                                                         (void**)&p_code);
                        }

                        p_old_ref = malloc((size_t)p_remote_type->size);
                        cops->read (p_cnx, p_old_ref, p_remote_type->size);

                        if (!new_data) {
                                _gs_alloc_ref(p_refs, p_local_type->size, p_old_ptr, old_ptr_length, p_new_ptr);
                                new_data = *p_new_ptr;
                        }

                        if (!_gs_is_null(p_old_ref, p_remote_type->size)) {
                                if (!(gras_dict_retrieve_ext(*p_refs, (char *)p_old_ref, p_remote_type->size, (void **)&p_new_ref), p_new_ref)) {
                                        void             *new_ref	= NULL;
                                        struct s_gs_type *p_ref_type	= NULL;

                                        fprintf(stderr, "receiving data referenced at %p\n", *(void **)p_old_ref);
                                        p_ref_type = _sg_message_receive_get_type_by_code(p_bag, p_cnx, code);

                                        _gs_message_receive_type(p_bag,
                                                                 p_cnx,
                                                                 p_refs,
                                                                 p_ref_type,
                                                                 p_old_ref,
                                                                 p_remote_type->size,
                                                                 &new_ref);

                                        *(void **)new_data	= new_ref;
                                } else {
                                        fprintf(stderr, "not receiving data referenced at %p\n", *(void **)p_old_ref);
                                        *(void **)new_data	= *p_new_ref;
                                }
                        } else {
                                fprintf(stderr, "not receiving data referenced at %p\n", *(void **)p_old_ref);
                                *(void **)new_data	= NULL;
                        }
                }
                break;

        case e_gs_type_cat_array    :
                {
                        struct s_gs_cat_array *p_array        =	NULL;
                        long int               count          =    0;
                        struct s_gs_type      *p_element_type =	NULL;
                        struct s_gs_type      *p_local_element_type =	NULL;
                        long int               element_size   =    0;
                        char                  *ptr            =	NULL;

                        p_array = p_remote_type->category.array_data;

                        {
                                long int *p_count = &count;

                                _gs_message_receive_type(p_bag,
                                                         p_cnx,
                                                         p_refs,
                                                         bops->get_type_by_name(p_bag, p_cnx, "signed long int"),
                                                         NULL,
                                                         0,
                                                         (void**)&p_count);
                        }

                        p_element_type		= _sg_message_receive_get_type_by_code(p_bag, p_cnx, p_array->code);
                        p_local_element_type	= bops->get_type_by_name(p_bag, NULL, p_element_type->name);
                        element_size		= p_local_element_type->aligned_size;

                        if (!new_data) {
                                _gs_alloc_ref(p_refs, count*element_size, p_old_ptr, old_ptr_length, p_new_ptr);
                                new_data = *p_new_ptr;
                        }

                        ptr = new_data;

                        if (!count)
                                return;

                        while (count --) {
                                _gs_message_receive_type(p_bag,
                                                         p_cnx,
                                                         p_refs,
                                                         p_element_type,
                                                         NULL,
                                                         0,
                                                         (void**)&ptr);

                                ptr += element_size;
                        }
                }
                break;

        case e_gs_type_cat_ignored:
                {
                        struct s_gs_cat_ignored *p_ignored        =	NULL;

                        if (!new_data) {
                                _gs_alloc_ref(p_refs, p_local_type->size, p_old_ptr, old_ptr_length, p_new_ptr);
                                new_data = *p_new_ptr;
                        }

                        p_ignored = p_local_type->category.ignored_data;

                        if (p_ignored->default_value) {
                                memcpy(new_data, p_ignored->default_value, (size_t)p_local_type->size);
                        }
                }
                break;


        default:
                GS_FAILURE("invalid type");
        }
}

struct s_gs_message_instance *
gs_message_init_receive(struct s_gs_type_bag	*p_bag,
                        struct s_gs_connection	*p_cnx) {

        struct s_gs_type_bag_ops	*bops			= p_bag->bag_ops;
        struct s_gs_message_instance	*p_message_instance	= NULL;
        int				*p_code			= NULL;
        struct s_gs_message		*p_message		= NULL;

        {
                gras_dict_t		*_refs		= NULL;
                struct s_gs_type 	*signed_int	= NULL;

                signed_int = bops->get_type_by_name(p_bag, p_cnx, "signed int");
                _gs_message_receive_type(p_bag, p_cnx, &_refs, signed_int, NULL, 0, (void**)&p_code);

                p_message = bops->get_message_by_code(p_bag, p_cnx, *p_code);

                if (!p_message) {
                        struct s_gs_type	*p_message_type = NULL;
                        p_message_type = bops->get_type_by_name(p_bag, p_cnx, "_s_gs_message");

                        if (!p_message_type)
                                GS_FAILURE("_s_gs_message default type missing");

                        _gs_message_receive_type(p_bag, p_cnx, &_refs, p_message_type, NULL, 0, (void **)&p_message);

                        bops->store_incoming_message(p_bag, p_cnx, p_message);
                }
        }

        p_message_instance	= _gs_message_instance_new();

        p_message_instance->p_message   	= p_message;
        p_message_instance->current		= p_message->first;
        p_message_instance->p_bag		= p_bag;
        p_message_instance->p_connection	= p_cnx;

        return p_message_instance;
}

void *
gs_message_receive_next_sequence(struct s_gs_message_instance *p_message_instance) {
        struct s_gs_type_bag		*p_bag		= p_message_instance->p_bag;
        struct s_gs_connection		*p_cnx		= p_message_instance->p_connection;
        struct s_gs_sequence		*p_sequence	= NULL;
        struct s_gs_type		*p_remote_type	= NULL;
        gras_dict_t			*refs		= NULL;
        void				*data		= NULL;

        p_sequence = p_message_instance->current;
        if (!p_sequence)
                GS_FAILURE("message type has no more sequences");

        p_remote_type = _sg_message_receive_get_type_by_code(p_bag, p_cnx, p_sequence->code);
        _gs_message_receive_type(p_bag,
                                 p_cnx,
                                 &refs,
                                 p_remote_type,
                                 NULL,
                                 0,
                                 &data);

        gras_dict_free(&refs);

        p_message_instance->current = p_sequence->next;

        return data;
}

