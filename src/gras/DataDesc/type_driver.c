/* gs_type_driver.c */

#include "DataDesc/gs_private.h"

static
gras_dict_t *p_type_driver_dict = NULL;

//GRAS_LOG_NEW_DEFAULT_CATEGORY(FIXME_type_driver);

void
gs_type_drivers_init(void) {
  gras_error_t errcode;
  
  TRYFAIL(gras_dict_new(&p_type_driver_dict));
  TRYFAIL(gras_dict_insert(p_type_driver_dict, "rl", gs_rl_type_driver(), NULL));
}


struct s_gs_type_driver *
gs_type_driver_init(const char *name) {

        struct s_gs_type_driver_ops	*type_driver_ops = NULL;
        struct s_gs_type_driver 	*p_type_driver   = NULL;

        gras_dict_retrieve(p_type_driver_dict, name, (void **)&type_driver_ops);
        if (!type_driver_ops)
                GS_FAILURE("type driver not found");

        p_type_driver		= calloc(1, sizeof (struct s_gs_type_driver));
        p_type_driver->type_ops	= type_driver_ops;
        p_type_driver->specific	= NULL;

        if (type_driver_ops->_init) {
                type_driver_ops->_init(p_type_driver);
        }

        return p_type_driver;
}



void
gs_type_driver_exit(struct s_gs_type_driver	*p_type_driver){

        struct s_gs_type_driver_ops	*type_driver_ops = NULL;

        type_driver_ops = p_type_driver->type_ops;
        if (type_driver_ops->_exit) {
                type_driver_ops->_exit(p_type_driver);
        }

        p_type_driver->type_ops = NULL;

        if (p_type_driver->specific) {
                free(p_type_driver->specific);
                p_type_driver->specific = NULL;
        }

        free(p_type_driver);
}

struct s_gs_type_bag *
gs_type_bag_alloc(struct s_gs_type_driver	*p_driver) {

        struct s_gs_type_bag		*p_bag	 = NULL;
        struct s_gs_type_bag_ops	*bag_ops = p_driver->bag_ops;

        p_bag = calloc(1, sizeof(struct s_gs_type_bag));

        p_bag->bag_ops		= bag_ops;
        p_bag->p_type_driver	= p_driver;
        p_bag->specific		= NULL;

        if (bag_ops->_init) {
                bag_ops->_init(p_bag);
        }

        return p_bag;
}

void
gs_type_bag_free(struct s_gs_type_bag	*p_bag) {

        struct s_gs_type_bag_ops	*bag_ops = p_bag->bag_ops;

        if (bag_ops->_exit) {
                bag_ops->_exit(p_bag);
        }

        if (p_bag->specific) {
                free(p_bag->specific);
                p_bag->specific = NULL;
        }

        free(p_bag);
}

