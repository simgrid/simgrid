/* gs_connection.h */
#ifndef GS_CONNECTION_H
#define GS_CONNECTION_H

/* used structs */
struct s_gs_connection;
struct s_gs_net_driver;
struct s_gs_type_bag;

enum e_gs_connection_direction {
        e_gs_connection_direction_unknown = 0,
        e_gs_connection_direction_outgoing,
        e_gs_connection_direction_incoming
};

struct s_gs_connection_ops {

        void
        (*_init)	(struct s_gs_connection	*p_connection,
                         void			*arg);

        void
        (*_exit)	(struct s_gs_connection	*p_connection);

        void
        (*write)	(struct s_gs_connection	*p_connection,
                         void			*ptr,
                         long int		 length);

        void
        (*read)		(struct s_gs_connection	*p_connection,
                         void			*ptr,
                         long int		 length);

        void
        (*flush)	(struct s_gs_connection	*p_connection);
};

struct s_gs_connection {
        struct s_gs_connection_ops	*connection_ops;
        struct s_gs_net_driver		*p_net_driver;
        enum e_gs_connection_direction   direction;
        void				*specific;
};


#endif /* GS_CONNECTION_H */
