/* gs_net_driver.h */
#ifndef GS_NET_DRIVER_H
#define GS_NET_DRIVER_H

/* used structs */
struct s_gs_connection;
struct s_gs_net_driver;

struct s_gs_net_driver_ops {

        void
        (*_init)	(struct s_gs_net_driver	*p_net_driver);

        void
        (*_exit)	(struct s_gs_net_driver	*p_net_driver);
};

struct s_gs_net_driver {
        struct s_gs_net_driver_ops	*net_ops;
        struct s_gs_connection_ops	*connection_ops;
        void				*specific;
};


#endif /* GS_NET_DRIVER_H */
