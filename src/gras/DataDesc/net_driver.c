/* gs_net_driver.c */

#include "DataDesc/gs_private.h"

//GRAS_LOG_NEW_DEFAULT_CATEGORY(FIXME_net_driver);

static
gras_dict_t *p_net_driver_dict = NULL;

void
gs_net_drivers_init(void) {
  gras_error_t errcode;

  TRYFAIL(gras_dict_new(&p_net_driver_dict));
  TRYFAIL(gras_dict_insert(p_net_driver_dict, "fd", gs_fd_net_driver(), NULL));
}


struct s_gs_net_driver *
gs_net_driver_init(const char *name) {

        struct s_gs_net_driver_ops	*net_driver_ops = NULL;
        struct s_gs_net_driver 		*p_net_driver   = NULL;

        gras_dict_retrieve(p_net_driver_dict, name, (void **)&net_driver_ops);
        if (!net_driver_ops)
                GS_FAILURE("net driver not found");

        p_net_driver			= calloc(1, sizeof (struct s_gs_net_driver));
        p_net_driver->net_ops		= net_driver_ops;
        p_net_driver->connection_ops	= NULL;
        p_net_driver->specific		= NULL;

        if (net_driver_ops->_init) {
                net_driver_ops->_init(p_net_driver);
        }

        return p_net_driver;
}



void
gs_net_driver_exit(struct s_gs_net_driver	*p_net_driver){

        struct s_gs_net_driver_ops	*net_driver_ops = NULL;

        net_driver_ops = p_net_driver->net_ops;
        if (net_driver_ops->_exit) {
                net_driver_ops->_exit(p_net_driver);
        }

        p_net_driver->net_ops = NULL;

        if (p_net_driver->specific) {
                free(p_net_driver->specific);
                p_net_driver->specific = NULL;
        }

        free(p_net_driver);
}

struct s_gs_connection *
gs_net_connection_connect(struct s_gs_net_driver	*p_net_driver,
                          void				*arg) {

        struct s_gs_connection_ops	*connection_ops = p_net_driver->connection_ops;
        struct s_gs_connection		*p_connection	= NULL;

        p_connection = malloc(sizeof (struct s_gs_connection));
        p_connection->connection_ops	= connection_ops;
        p_connection->p_net_driver	= p_net_driver;
        p_connection->direction		= e_gs_connection_direction_outgoing;

        if (connection_ops->_init) {
                connection_ops->_init(p_connection, arg);
        }

        return p_connection;
}

struct s_gs_connection *
gs_net_connection_accept(struct s_gs_net_driver	*p_net_driver,
                         void			*arg) {

        struct s_gs_connection_ops	*connection_ops = p_net_driver->connection_ops;
        struct s_gs_connection		*p_connection	= NULL;

        p_connection = malloc(sizeof (struct s_gs_connection));
        p_connection->connection_ops	= connection_ops;
        p_connection->p_net_driver	= p_net_driver;
        p_connection->direction		= e_gs_connection_direction_incoming;

        if (connection_ops->_init) {
                connection_ops->_init(p_connection, arg);
        }

        return p_connection;
}

void
gs_net_connection_close(struct s_gs_connection	*p_connection) {
        if (p_connection->connection_ops->_exit) {
                p_connection->connection_ops->_exit(p_connection);
        }

        if (p_connection->specific) {
                free(p_connection->specific);
                p_connection->specific = NULL;
        }

        free(p_connection);
}
