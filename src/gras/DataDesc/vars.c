/* gs_vars.c */

#include "gs/gs_private.h"

struct s_gs_var {
        struct s_gs_type	*p_type;
        void			*data;
};


struct s_gs_vars {
        gras_dict_t		*space;
        gras_dynar_t		*stack;
        gras_dynar_t		*globals;
};


static
void
_gs_vars_pop(struct s_gs_vars	*p_vars,
             const char		*name) {

        gras_dynar_t 		*p_dynar	= NULL;
        struct s_gs_var         *p_var		= NULL;

        gras_dict_retrieve(p_vars->space, name, (void **)&p_dynar);
        gras_dynar_pop(p_dynar, &p_var);

        if (!gras_dynar_length(p_dynar)) {
                gras_dict_remove(p_vars->space, name);
                gras_dynar_free_container(p_dynar);
        }

        free(p_var);
}

void *
gs_vars_pop(void		 *_p_vars,
            const char		 *name,
            struct s_gs_type	**pp_type) {

        struct s_gs_vars	*p_vars		= _p_vars;
        gras_dynar_t 		*p_dynar	= NULL;
        struct s_gs_var         *p_var		= NULL;
        void                    *data           = NULL;

        gras_dict_retrieve(p_vars->space, name, (void **)&p_dynar);
        gras_dynar_pop(p_dynar, &p_var);

        if (!gras_dynar_length(p_dynar)) {
                gras_dict_remove(p_vars->space, name);
                gras_dynar_free_container(p_dynar);
        }

        if (pp_type) {
                *pp_type = p_var->p_type;
        }

        data    = p_var->data;

        free(p_var);

        gras_dynar_pop(p_vars->stack, &p_dynar);
        {
                int l = gras_dynar_length(p_dynar);

                while (l--) {
                        char *_name = NULL;

                        gras_dynar_get(p_dynar, l, &_name);
                        if (!strcmp(name, _name)) {
                                gras_dynar_remove_at(p_dynar, l, &_name);
                                free(_name);
                                break;
                        }
                }
        }
        gras_dynar_push(p_vars->stack, &p_dynar);


        return data;
}

void
gs_vars_push(void		*_p_vars,
             struct s_gs_type	*p_type,
             const char		*name,
             void		*data) {

        struct s_gs_vars	*p_vars		= _p_vars;
        gras_dynar_t 		*p_dynar	= NULL;
        struct s_gs_var         *p_var		= NULL;

        name = strdup(name);

        gras_dict_retrieve(p_vars->space, name, (void **)&p_dynar);

        if (!p_dynar) {
                gras_dynar_new(&p_dynar, sizeof (struct s_gs_var *), NULL);
                gras_dict_insert(p_vars->space, name, (void **)p_dynar, NULL);
        }

        p_var		= calloc(1, sizeof(struct s_gs_var));
        p_var->p_type	= p_type;
        p_var->data	= data;

        gras_dynar_push(p_dynar, &p_var);

        gras_dynar_pop(p_vars->stack, &p_dynar);
        gras_dynar_push(p_dynar, &name);
        gras_dynar_push(p_vars->stack, &p_dynar);
}

void
gs_vars_set(void		*_p_vars,
             struct s_gs_type	*p_type,
             const char		*name,
             void		*data) {

        struct s_gs_vars	*p_vars		= _p_vars;
        gras_dynar_t 		*p_dynar	= NULL;
        struct s_gs_var         *p_var		= NULL;

        name = strdup(name);
        gras_dict_retrieve(p_vars->space, name, (void **)&p_dynar);

        if (!p_dynar) {
                gras_dynar_new(&p_dynar, sizeof (struct s_gs_var *), NULL);
                gras_dict_insert(p_vars->space, name, (void **)p_dynar, NULL);

                p_var	= calloc(1, sizeof(struct s_gs_var));
                gras_dynar_push(p_vars->globals, &name);
        } else {
                gras_dynar_pop(p_dynar, &p_var);
        }

        p_var->p_type	= p_type;
        p_var->data	= data;

        gras_dynar_push(p_dynar, &p_var);
}

void *
gs_vars_get(void		 *_p_vars,
            const char		 *name,
            struct s_gs_type	**pp_type) {

        struct s_gs_vars	*p_vars	= _p_vars;
        gras_dynar_t 		*p_dynar	= NULL;
        struct s_gs_var         *p_var		= NULL;

        gras_dict_retrieve(p_vars->space, name, (void **)&p_dynar);
        gras_dynar_pop(p_dynar, &p_var);
        gras_dynar_push(p_dynar, &p_var);

        if (pp_type) {
                *pp_type = p_var->p_type;
        }

        return p_var->data;
}

void
gs_vars_enter(void *_p_vars) {

        struct s_gs_vars	*p_vars		= _p_vars;
        gras_dynar_t 		*p_dynar	= NULL;

        gras_dynar_new(&p_dynar, sizeof (char *), NULL);
        gras_dynar_push(p_vars->stack, &p_dynar);
}


void
gs_vars_leave(void *_p_vars) {
        struct s_gs_vars	*p_vars		= _p_vars;
        gras_dynar_t 		*p_dynar	= NULL;
        int           		 cursor		=    0;
        char         		*name		= NULL;

        gras_dynar_pop(p_vars->stack, &p_dynar);

	gras_dynar_foreach(p_dynar, cursor, name) {
                _gs_vars_pop(p_vars, name);
                free(name);
        }

        gras_dynar_free_container(p_dynar);
}


void *
gs_vars_alloc(void) {

        struct s_gs_vars	*p_vars = NULL;

        p_vars = calloc(1, sizeof (struct s_gs_vars));

        gras_dict_new (&p_vars->space);
        gras_dynar_new(&p_vars->stack, sizeof (gras_dynar_t *), NULL);

        gs_vars_enter(p_vars);

        gras_dynar_pop(p_vars->stack, &p_vars->globals);
        gras_dynar_push(p_vars->stack, &p_vars->globals);

        return p_vars;
}


void
gs_vars_free(void *_p_vars) {

        struct s_gs_vars	*p_vars	= _p_vars;

        gs_vars_leave(p_vars);

        gras_dynar_free_container(p_vars->stack);
        gras_dict_free(&p_vars->space);

        free(p_vars);
}

