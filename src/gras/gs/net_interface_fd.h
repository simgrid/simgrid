/* gs_fd_net_interface.h */
#ifndef GS_FD_NET_INTERFACE_H
#define GS_FD_NET_INTERFACE_H


struct s_gs_fd_net_connection_arg {
        int fd;
};

struct s_gs_net_driver_ops *
gs_fd_net_driver(void);


void
gs_fd__init(struct s_gs_net_driver	*p_driver);


void
gs_fd__exit(struct s_gs_net_driver	*p_driver);


void
gs_fd_connection__init(struct s_gs_connection	*p_connection,
                       void			*arg);

void
gs_fd_connection__exit(struct s_gs_connection	*p_connection);

void
gs_fd_connection_write(struct s_gs_connection	*p_connection,
                       void			*ptr,
                       long int		 	 length);

void
gs_fd_connection_read(struct s_gs_connection	*p_connection,
                      void			*ptr,
                      long int		 	 length);
void
gs_fd_connection_flush(struct s_gs_connection	*p_connection);
#endif /* GS_FD_NET_INTERFACE_H */
