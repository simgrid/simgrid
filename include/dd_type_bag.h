/* gs_type_bag.h */
#ifndef GS_TYPE_BAG_H
#define GS_TYPE_BAG_H

/* used structs */
struct s_gs_type_bag;
struct s_gs_type_driver;

struct s_gs_type_bag_ops {

        void
        (*_init)			(struct s_gs_type_bag	*p_type_bag);

        void
        (*_exit)			(struct s_gs_type_bag	*p_type_bag);


        void
        (*register_incoming_connection)	(struct s_gs_type_bag	*p_type_bag,
                                         struct s_gs_connection	*p_connection);

        void
        (*register_outgoing_connection) (struct s_gs_type_bag	*p_type_bag,
                                         struct s_gs_connection	*p_connection);


        void
        (*store_type)			(struct s_gs_type_bag	*p_type_bag,
                        		 struct s_gs_connection	*p_connection,
                        		 struct s_gs_type	*p_type);

        void
        (*store_incoming_type)		(struct s_gs_type_bag	*p_type_bag,
                                         struct s_gs_connection	*p_connection,
                                         struct s_gs_type	*p_type);

        struct s_gs_type *
        (*get_type_by_name)		(struct s_gs_type_bag	*p_type_bag,
                                	 struct s_gs_connection	*p_connection,
                                	 const char		*name);

        struct s_gs_type *
        (*get_type_by_code)		(struct s_gs_type_bag	*p_type_bag,
                                	 struct s_gs_connection	*p_connection,
                                	 int			 code);

        void
        (*mark_type) 			(struct s_gs_type_bag	*p_type_bag,
                        		 struct s_gs_connection	*p_connection,
                        		 const char		*name);

        int
        (*check_type_mark) 		(struct s_gs_type_bag	*p_type_bag,
                               		 struct s_gs_connection	*p_connection,
                               		 const char		*name);


        void
        (*store_message)		(struct s_gs_type_bag	*p_type_bag,
                        		 struct s_gs_connection	*p_connection,
                        		 struct s_gs_message	*p_message);

        void
        (*store_incoming_message)	(struct s_gs_type_bag	*p_type_bag,
                                         struct s_gs_connection	*p_connection,
                                         struct s_gs_message	*p_message);

        struct s_gs_message *
        (*get_message_by_name)		(struct s_gs_type_bag	*p_type_bag,
                                	 struct s_gs_connection	*p_connection,
                                	 const char		*name);

        struct s_gs_message *
        (*get_message_by_code)		(struct s_gs_type_bag	*p_type_bag,
                                	 struct s_gs_connection	*p_connection,
                                	 int			 code);

        void
        (*mark_message) 		(struct s_gs_type_bag	*p_type_bag,
                        		 struct s_gs_connection	*p_connection,
                        		 const char		*name);

        int
        (*check_message_mark) 		(struct s_gs_type_bag	*p_type_bag,
                               		 struct s_gs_connection	*p_connection,
                               		 const char		*name);

};

struct s_gs_type_bag {
        struct s_gs_type_bag_ops	*bag_ops;
        struct s_gs_type_driver		*p_type_driver;
        void				*specific;
};


#endif /* GS_TYPE_BAG_H */
