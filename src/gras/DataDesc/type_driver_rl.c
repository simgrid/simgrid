/* gs_rl_type_driver.c */

#include "DataDesc/gs_private.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(NDR_tdriver_rl,NDR);
/*
 * structs
 */
struct s_gs_rl_type_driver {
        /**/
        int dummy;
};

struct s_gs_rl_type_bag {
        gras_dict_t  		*incoming_dict;

        gras_dynar_t 		*type_dynar;
        gras_dict_t  		*type_dict;

        gras_dynar_t		*message_dynar;
        gras_dict_t		*message_dict;

        gras_dict_t  		*outgoing_dict;
};

struct s_gs_rl_incoming_connection {
        struct s_gs_connection	*p_connection;

        gras_dynar_t		*type_dynar;
        gras_dict_t		*type_dict;

        gras_dynar_t		*message_dynar;
        gras_dict_t		*message_dict;
};

struct s_gs_rl_outgoing_connection {
        struct s_gs_connection	*p_connection;
        gras_dict_t		*type_marker_dict;
        gras_dict_t		*message_marker_dict;
};


/*
 * static vars
 */
static
struct s_gs_type_driver_ops *type_driver_ops = NULL;

static
struct s_gs_type_bag_ops    *type_bag_ops    = NULL;

/*
 * exported functions, driver part
 */
struct s_gs_type_driver_ops *
gs_rl_type_driver(void) {

        if (!type_bag_ops) {
                type_bag_ops	= calloc(1, sizeof(struct s_gs_type_bag_ops));


                type_bag_ops->_init				= gs_rl_bag__init;
                type_bag_ops->_exit				= gs_rl_bag__exit;


                type_bag_ops->register_incoming_connection	= gs_rl_bag_register_incoming_connection;
                type_bag_ops->register_outgoing_connection	= gs_rl_bag_register_outgoing_connection;


                type_bag_ops->store_type			= gs_rl_bag_store_type;
                type_bag_ops->store_incoming_type		= gs_rl_bag_store_incoming_type;
                type_bag_ops->get_type_by_name			= gs_rl_bag_get_type_by_name;
                type_bag_ops->get_type_by_code			= gs_rl_bag_get_type_by_code;
                type_bag_ops->mark_type				= gs_rl_bag_mark_type;
                type_bag_ops->check_type_mark			= gs_rl_bag_check_type_mark;


                type_bag_ops->store_message			= gs_rl_bag_store_message;
                type_bag_ops->store_incoming_message		= gs_rl_bag_store_incoming_message;
                type_bag_ops->get_message_by_name		= gs_rl_bag_get_message_by_name;
                type_bag_ops->get_message_by_code		= gs_rl_bag_get_message_by_code;
                type_bag_ops->mark_message			= gs_rl_bag_mark_message;
                type_bag_ops->check_message_mark		= gs_rl_bag_check_message_mark;
        }

        if (!type_driver_ops) {
                type_driver_ops = calloc(1, sizeof(struct s_gs_type_driver_ops));

                type_driver_ops->_init	= gs_rl__init;
                type_driver_ops->_exit	= gs_rl__exit;
        }

        return type_driver_ops;
}

void
gs_rl__init(struct s_gs_type_driver	*p_driver) {

        struct s_gs_rl_type_driver *p_rl = NULL;

        p_rl = calloc(1, sizeof(struct s_gs_rl_type_driver));
        p_driver->specific	= p_rl;
        p_driver->bag_ops	= type_bag_ops;
}

void
gs_rl__exit(struct s_gs_type_driver	*p_driver) {

        struct s_gs_rl_type_driver *p_rl = p_driver->specific;

        free(p_rl);
        p_driver->specific = NULL;
}

/*
 * exported functions, bag part
 */

/* management */
void
gs_rl_bag__init(struct s_gs_type_bag	*p_bag) {

        struct s_gs_rl_type_bag *p_rl = NULL;

        p_rl = calloc(1, sizeof(struct s_gs_rl_type_bag));

        gras_dict_new(&p_rl->incoming_dict);

        gras_dynar_new(&p_rl->type_dynar, sizeof(struct s_gs_type *), NULL);
	gras_dict_new (&p_rl->type_dict);

        gras_dynar_new(&p_rl->message_dynar, sizeof(struct s_gs_type *), NULL);
	gras_dict_new (&p_rl->message_dict);

        gras_dict_new (&p_rl->outgoing_dict);

        p_bag->specific		= p_rl;
        p_bag->bag_ops		= type_bag_ops;

        gs_bootstrap_type_bag(p_bag);
}

void
gs_rl_bag__exit(struct s_gs_type_bag	*p_bag) {

        struct s_gs_rl_type_bag *p_rl = p_bag->specific;

        gras_dict_free(&p_rl->type_dict);
        gras_dynar_free(p_rl->type_dynar);

        free(p_rl);
        p_bag->specific = NULL;
}

/* connection */
void
gs_rl_bag_register_incoming_connection(struct s_gs_type_bag	*p_bag,
                                       struct s_gs_connection	*p_cnx) {

        struct s_gs_rl_type_bag			*p_rl	= p_bag->specific;
        struct s_gs_rl_incoming_connection	*p_in	= NULL;

        if (p_cnx->direction != e_gs_connection_direction_incoming)
                GS_FAILURE("invalid operation");

        p_in = malloc(sizeof (struct s_gs_rl_incoming_connection));

        p_in->p_connection	= p_cnx;

        gras_dynar_new(&p_in->type_dynar, sizeof(struct s_gs_type *), NULL);
        gras_dict_new (&p_in->type_dict);

        gras_dynar_new(&p_in->message_dynar, sizeof(struct s_gs_type *), NULL);
        gras_dict_new (&p_in->message_dict);

        gras_dict_insert_ext(p_rl->incoming_dict,
                             (char *)&p_cnx,
                             sizeof(p_cnx),
                             p_in,
                             NULL);

        gs_bootstrap_incoming_connection(p_bag, p_cnx);
}


void
gs_rl_bag_register_outgoing_connection(struct s_gs_type_bag	*p_bag,
                                       struct s_gs_connection	*p_cnx) {

        struct s_gs_rl_type_bag			*p_rl	= p_bag->specific;
        struct s_gs_rl_outgoing_connection	*p_out	= NULL;

        if (p_cnx->direction != e_gs_connection_direction_outgoing)
                GS_FAILURE("invalid operation");

        p_out = malloc(sizeof (struct s_gs_rl_outgoing_connection));

        p_out->p_connection	= p_cnx;
        gras_dict_new(&p_out->type_marker_dict);
        gras_dict_new(&p_out->message_marker_dict);

        gras_dict_insert_ext(p_rl->outgoing_dict, (char *)&p_cnx, sizeof(p_cnx), p_out, NULL);

        gs_bootstrap_outgoing_connection(p_bag, p_cnx);
}


/* types */
void
gs_rl_bag_store_type(struct s_gs_type_bag	*p_bag,
                     struct s_gs_connection	*p_cnx,
                     struct s_gs_type		*p_type) {

        struct s_gs_rl_type_bag  *p_rl 		= p_bag->specific;
        struct s_gs_type 	**pp_type	= NULL;

        pp_type	 = malloc(sizeof(struct s_gs_type *));
        *pp_type = p_type;

        if (p_cnx) {
                struct s_gs_rl_incoming_connection	*p_in	= NULL;

                if (p_cnx->direction != e_gs_connection_direction_incoming)
                        GS_FAILURE("invalid operation");

                gras_dict_retrieve_ext(p_rl->incoming_dict,
                                       (char *)&p_cnx,
                                       sizeof(p_cnx),
                                       (void **)&p_in);


                p_type->code = gras_dynar_length(p_in->type_dynar);
                gras_dynar_insert_at(p_in->type_dynar, p_type->code, pp_type);
                gras_dict_insert(p_in->type_dict, p_type->name, pp_type, NULL);
        } else {
                p_type->code = gras_dynar_length(p_rl->type_dynar);
                gras_dynar_insert_at(p_rl->type_dynar, p_type->code, pp_type);
                gras_dict_insert(p_rl->type_dict, p_type->name, pp_type, NULL);
        }
}

void
gs_rl_bag_store_incoming_type(struct s_gs_type_bag	*p_bag,
                              struct s_gs_connection	*p_cnx,
                              struct s_gs_type		*p_type) {

        struct s_gs_rl_type_bag			 *p_rl 		= p_bag->specific;
        struct s_gs_rl_incoming_connection	 *p_in		= NULL;
        struct s_gs_type 			**pp_type	= NULL;

        if (p_cnx->direction != e_gs_connection_direction_incoming)
                GS_FAILURE("invalid operation");

        gras_dict_retrieve_ext(p_rl->incoming_dict,
                               (char *)&p_cnx,
                               sizeof(p_cnx),
                               (void **)&p_in);

        pp_type	 = malloc(sizeof(struct s_gs_type *));
        *pp_type = p_type;

        gras_dynar_set(p_in->type_dynar, p_type->code, pp_type);
        gras_dict_insert(p_in->type_dict, p_type->name, pp_type, NULL);
}

struct s_gs_type *
gs_rl_bag_get_type_by_name(struct s_gs_type_bag		*p_bag,
                           struct s_gs_connection	*p_cnx,
                           const char			*name) {

        struct s_gs_rl_type_bag	 *p_rl		= p_bag->specific;
        struct s_gs_type	**pp_type	= NULL;

        if (p_cnx) {
                struct s_gs_rl_incoming_connection	 *p_in		= NULL;

                if (p_cnx->direction != e_gs_connection_direction_incoming)
                        GS_FAILURE("invalid operation");

                gras_dict_retrieve_ext(p_rl->incoming_dict,
                                       (char *)&p_cnx,
                                       sizeof(p_cnx),
                                       (void **)&p_in);

                gras_dict_retrieve(p_in->type_dict, name, (void **)&pp_type);
        } else {
                gras_dict_retrieve(p_rl->type_dict, name, (void **)&pp_type);
        }

        if (!pp_type) {
	        DEBUG1("Get type by name '%s': not found",name);
                return NULL;
        }

	DEBUG1("Get type by name '%s': found",name);
        return *pp_type;
}

struct s_gs_type *
gs_rl_bag_get_type_by_code(struct s_gs_type_bag		*p_bag,
                           struct s_gs_connection	*p_cnx,
                           int				 code) {

        struct s_gs_rl_type_bag	*p_rl	= p_bag->specific;
        struct s_gs_type	*p_type	= NULL;

        if (p_cnx) {
                struct s_gs_rl_incoming_connection	 *p_in		= NULL;

                if (p_cnx->direction != e_gs_connection_direction_incoming)
                        GS_FAILURE("invalid operation");

                gras_dict_retrieve_ext(p_rl->incoming_dict,
                                       (char *)&p_cnx,
                                       sizeof(p_cnx),
                                       (void **)&p_in);

                if ((unsigned int)code < gras_dynar_length(p_in->type_dynar)) {
                        gras_dynar_get(p_in->type_dynar, code, (void **)&p_type);
                }
        } else {
                if ((unsigned int)code < gras_dynar_length(p_rl->type_dynar)) {
                        gras_dynar_get(p_rl->type_dynar, code, (void **)&p_type);
                }
        }

        return p_type;
}

void
gs_rl_bag_mark_type(struct s_gs_type_bag	*p_bag,
                    struct s_gs_connection	*p_cnx,
                    const char			*name) {

        struct s_gs_rl_type_bag			*p_rl	= p_bag->specific;
        struct s_gs_rl_outgoing_connection	*p_out	= NULL;

        if (p_cnx->direction != e_gs_connection_direction_outgoing)
                GS_FAILURE("invalid operation");

        gras_dict_retrieve_ext(p_rl->outgoing_dict, (char *)&p_cnx, sizeof(p_cnx), (void **)&p_out);

        if (!p_out)
                GS_FAILURE("unregistered connection");

        gras_dict_insert(p_out->type_marker_dict, name, strdup(name), NULL);
}

int
gs_rl_bag_check_type_mark(struct s_gs_type_bag		*p_bag,
                          struct s_gs_connection	*p_cnx,
                          const char			*name) {

        struct s_gs_rl_type_bag			*p_rl	= p_bag->specific;
        struct s_gs_rl_outgoing_connection	*p_out	= NULL;
        char                                    *result = NULL;

        if (p_cnx->direction != e_gs_connection_direction_outgoing)
                GS_FAILURE("invalid operation");

        gras_dict_retrieve_ext(p_rl->outgoing_dict, (char *)&p_cnx, sizeof(p_cnx), (void **)&p_out);

        if (!p_out)
                GS_FAILURE("unregistered connection");

        gras_dict_retrieve(p_out->type_marker_dict, name, (void **)&result);

        return !!result;
}

/* messages */
void
gs_rl_bag_store_message(struct s_gs_type_bag	*p_bag,
                        struct s_gs_connection	*p_cnx,
                        struct s_gs_message	*p_message) {

        struct s_gs_rl_type_bag  *p_rl 		= p_bag->specific;
        struct s_gs_message 	**pp_message	= NULL;

        pp_message	 = malloc(sizeof(struct s_gs_message *));
        *pp_message = p_message;

        if (p_cnx) {
                struct s_gs_rl_incoming_connection	*p_in	= NULL;

                if (p_cnx->direction != e_gs_connection_direction_incoming)
                        GS_FAILURE("invalid operation");

                gras_dict_retrieve_ext(p_rl->incoming_dict,
                                       (char *)&p_cnx,
                                       sizeof(p_cnx),
                                       (void **)&p_in);


                p_message->code = gras_dynar_length(p_in->message_dynar);
                gras_dynar_insert_at(p_in->message_dynar, p_message->code, pp_message);
                gras_dict_insert(p_in->message_dict, p_message->name, pp_message, NULL);
        } else {
                p_message->code = gras_dynar_length(p_rl->message_dynar);
                gras_dynar_insert_at(p_rl->message_dynar, p_message->code, pp_message);
                gras_dict_insert(p_rl->message_dict, p_message->name, pp_message, NULL);
        }
}

void
gs_rl_bag_store_incoming_message(struct s_gs_type_bag	*p_bag,
                                 struct s_gs_connection	*p_cnx,
                                 struct s_gs_message	*p_message) {

        struct s_gs_rl_type_bag			 *p_rl 		= p_bag->specific;
        struct s_gs_rl_incoming_connection	 *p_in		= NULL;
        struct s_gs_message 			**pp_message	= NULL;

        if (p_cnx->direction != e_gs_connection_direction_incoming)
                GS_FAILURE("invalid operation");

        gras_dict_retrieve_ext(p_rl->incoming_dict,
                               (char *)&p_cnx,
                               sizeof(p_cnx),
                               (void **)&p_in);

        pp_message	 = malloc(sizeof(struct s_gs_message *));
        *pp_message = p_message;

        gras_dynar_set(p_in->message_dynar, p_message->code, pp_message);
        gras_dict_insert(p_in->message_dict, p_message->name, pp_message, NULL);
}

struct s_gs_message *
gs_rl_bag_get_message_by_name(struct s_gs_type_bag	*p_bag,
                              struct s_gs_connection	*p_cnx,
                              const char		*name) {

        struct s_gs_rl_type_bag	 *p_rl		= p_bag->specific;
        struct s_gs_message	**pp_message	= NULL;

        if (p_cnx) {
                struct s_gs_rl_incoming_connection	 *p_in		= NULL;

                if (p_cnx->direction != e_gs_connection_direction_incoming)
                        GS_FAILURE("invalid operation");

                gras_dict_retrieve_ext(p_rl->incoming_dict,
                                       (char *)&p_cnx,
                                       sizeof(p_cnx),
                                       (void **)&p_in);

                gras_dict_retrieve(p_in->message_dict, name, (void **)&pp_message);
        } else {
                gras_dict_retrieve(p_rl->message_dict, name, (void **)&pp_message);
        }

        if (!pp_message) {
                return NULL;
        }

        return *pp_message;
}

struct s_gs_message *
gs_rl_bag_get_message_by_code(struct s_gs_type_bag	*p_bag,
                              struct s_gs_connection	*p_cnx,
                              int			 code) {

        struct s_gs_rl_type_bag	*p_rl	= p_bag->specific;
        struct s_gs_message	*p_message	= NULL;

        if (p_cnx) {
                struct s_gs_rl_incoming_connection	 *p_in		= NULL;

                if (p_cnx->direction != e_gs_connection_direction_incoming)
                        GS_FAILURE("invalid operation");

                gras_dict_retrieve_ext(p_rl->incoming_dict,
                                       (char *)&p_cnx,
                                       sizeof(p_cnx),
                                       (void **)&p_in);

                if ((unsigned int)code < gras_dynar_length(p_in->message_dynar)) {
                        gras_dynar_get(p_in->message_dynar, code, (void **)&p_message);
                }
        } else {
                if ((unsigned int)code < gras_dynar_length(p_rl->message_dynar)) {
                        gras_dynar_get(p_rl->message_dynar, code, (void **)&p_message);
                }
        }

        return p_message;
}

void
gs_rl_bag_mark_message(struct s_gs_type_bag	*p_bag,
                       struct s_gs_connection	*p_cnx,
                       const char		*name) {

        struct s_gs_rl_type_bag			*p_rl	= p_bag->specific;
        struct s_gs_rl_outgoing_connection	*p_out	= NULL;

        if (p_cnx->direction != e_gs_connection_direction_outgoing)
                GS_FAILURE("invalid operation");

        gras_dict_retrieve_ext(p_rl->outgoing_dict, (char *)&p_cnx, sizeof(p_cnx), (void **)&p_out);

        if (!p_out)
                GS_FAILURE("unregistered connection");

        gras_dict_insert(p_out->message_marker_dict, name, strdup(name), NULL);
}

int
gs_rl_bag_check_message_mark(struct s_gs_type_bag	*p_bag,
                             struct s_gs_connection	*p_cnx,
                             const char			*name) {

        struct s_gs_rl_type_bag			*p_rl	= p_bag->specific;
        struct s_gs_rl_outgoing_connection	*p_out	= NULL;
        char                                    *result = NULL;

        if (p_cnx->direction != e_gs_connection_direction_outgoing)
                GS_FAILURE("invalid operation");

        gras_dict_retrieve_ext(p_rl->outgoing_dict, (char *)&p_cnx, sizeof(p_cnx), (void **)&p_out);

        if (!p_out)
                GS_FAILURE("unregistered connection");

        gras_dict_retrieve(p_out->message_marker_dict, name, (void **)&result);

        return !!result;
}


