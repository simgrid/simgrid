/* gs_fd_net_driver.c */

#include "gs/gs_private.h"

struct s_gs_fd_net_driver {
        /**/
        int dummy;
};

struct s_gs_fd_connection {
        int fd;
};

static
struct s_gs_net_driver_ops *net_driver_ops = NULL;

static
struct s_gs_connection_ops *connection_ops = NULL;

struct s_gs_net_driver_ops *
gs_fd_net_driver(void) {
        if (!connection_ops) {
                connection_ops = calloc(1, sizeof(struct s_gs_connection_ops));

                connection_ops->_init	= gs_fd_connection__init;
                connection_ops->_exit	= gs_fd_connection__exit;
                connection_ops->write	= gs_fd_connection_write;
                connection_ops->read	= gs_fd_connection_read;
                connection_ops->flush	= gs_fd_connection_flush;
        }

        if (!net_driver_ops) {
                net_driver_ops = calloc(1, sizeof(struct s_gs_net_driver_ops));

                net_driver_ops->_init	= gs_fd__init;
                net_driver_ops->_exit	= gs_fd__exit;
        }

        return net_driver_ops;
}

void
gs_fd__init(struct s_gs_net_driver	*p_net_driver) {

        struct s_gs_fd_net_driver *p_fd = NULL;

        p_fd = calloc(1, sizeof(struct s_gs_fd_net_driver));
        p_net_driver->connection_ops	= connection_ops;
        p_net_driver->specific		= p_fd;
}

void
gs_fd__exit(struct s_gs_net_driver	*p_net_driver) {

        struct s_gs_fd_net_driver *p_fd = p_net_driver->specific;

        free(p_fd);

        p_net_driver->specific = NULL;
}


void
gs_fd_connection__init(struct s_gs_connection	*p_connection,
                       void			*arg) {

        struct s_gs_fd_net_connection_arg	*p_fd_arg = arg;
        struct s_gs_fd_connection		*p_fd_cnx = NULL;

        p_fd_cnx		= calloc(1, sizeof(struct s_gs_fd_connection));
        p_fd_cnx->fd		= p_fd_arg->fd;
        p_connection->specific	= p_fd_cnx;
}

void
gs_fd_connection__exit(struct s_gs_connection	*p_connection) {
        struct s_gs_fd_connection *p_fd_cnx = p_connection->specific;

        free(p_fd_cnx);

        p_connection->specific = NULL;
}

void
gs_fd_connection_write(struct s_gs_connection	*p_connection,
                       void			*_ptr,
                       long int		 	 length) {
        struct s_gs_fd_connection *p_fd_cnx = p_connection->specific;
        unsigned char		  *ptr      = _ptr;

        if (p_connection->direction != e_gs_connection_direction_outgoing)
                GS_FAILURE("invalid operation");

        while (length) {
                int status = 0;

                status = write(p_fd_cnx->fd, ptr, (size_t)length);
                fprintf(stderr, "write(%d, %p, %ld);\n", p_fd_cnx->fd, ptr, length);

                if (status == -1) {
                        perror("write");
                        GS_FAILURE("system call failed");
                }

                if (status) {
                        length	-= status;
                        ptr	+= status;
                } else {
                        GS_FAILURE("file descriptor closed");
                }
        }
}

void
gs_fd_connection_read(struct s_gs_connection	*p_connection,
                      void			*_ptr,
                      long int		 	 length) {
        struct s_gs_fd_connection *p_fd_cnx = p_connection->specific;
        unsigned char		  *ptr      = _ptr;

        if (p_connection->direction != e_gs_connection_direction_incoming)
                GS_FAILURE("invalid operation");

        while (length) {
                int status = 0;

                status = read(p_fd_cnx->fd, ptr, (size_t)length);
                fprintf(stderr, "read(%d, %p, %ld);\n", p_fd_cnx->fd, ptr, length);

                if (status == -1) {
                        perror("write");
                        GS_FAILURE("system call failed");
                }

                if (status) {
                        length	-= status;
                        ptr	+= status;
                } else {
                        GS_FAILURE("file descriptor closed");
                }
        }
}
void
gs_fd_connection_flush(struct s_gs_connection	*p_connection) {
        (void)p_connection;

        /* */
}

