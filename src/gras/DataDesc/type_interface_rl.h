/* gs_rl_type_interface.h */
#ifndef GS_RL_TYPE_INTERFACE_H
#define GS_RL_TYPE_INTERFACE_H

struct s_gs_type_driver_ops *
gs_rl_type_driver(void);



void
gs_rl__init(struct s_gs_type_driver	*p_driver);

void
gs_rl__exit(struct s_gs_type_driver	*p_driver);



void
gs_rl_bag__init(struct s_gs_type_bag	*p_bag);

void
gs_rl_bag__exit(struct s_gs_type_bag	*p_bag);



void
gs_rl_bag_register_incoming_connection	(struct s_gs_type_bag	*p_bag,
                                      	 struct s_gs_connection	*p_connection);

void
gs_rl_bag_register_outgoing_connection	(struct s_gs_type_bag	*p_bag,
                                      	 struct s_gs_connection	*p_connection);



void
gs_rl_bag_store_type			(struct s_gs_type_bag	*p_bag,
                    			 struct s_gs_connection	*p_connection,
                    			 struct s_gs_type	*p_type);

void
gs_rl_bag_store_incoming_type		(struct s_gs_type_bag	*p_bag,
                             		 struct s_gs_connection	*p_cnx,
                             		 struct s_gs_type	*p_type);

struct s_gs_type *
gs_rl_bag_get_type_by_name		(struct s_gs_type_bag	*p_bag,
                          		 struct s_gs_connection	*p_connection,
                          		 const char		*name);

struct s_gs_type *
gs_rl_bag_get_type_by_code		(struct s_gs_type_bag	*p_bag,
                          		 struct s_gs_connection	*p_connection,
                          		 int			 code);

void
gs_rl_bag_mark_type			(struct s_gs_type_bag	*p_bag,
                   			 struct s_gs_connection	*p_connection,
                   			 const char		*name);

int
gs_rl_bag_check_type_mark		(struct s_gs_type_bag	*p_bag,
                         		 struct s_gs_connection	*p_connection,
                         		 const char		*name);



void
gs_rl_bag_store_message			(struct s_gs_type_bag	*p_bag,
                       			 struct s_gs_connection	*p_connection,
                       			 struct s_gs_message	*p_message);

void
gs_rl_bag_store_incoming_message	(struct s_gs_type_bag	*p_bag,
                                	 struct s_gs_connection	*p_cnx,
                                	 struct s_gs_message	*p_message);

struct s_gs_message *
gs_rl_bag_get_message_by_name		(struct s_gs_type_bag	*p_bag,
                             		 struct s_gs_connection	*p_connection,
                             		 const char			*name);

struct s_gs_message *
gs_rl_bag_get_message_by_code		(struct s_gs_type_bag	*p_bag,
                             		 struct s_gs_connection	*p_connection,
                             		 int			 code);

void
gs_rl_bag_mark_message			(struct s_gs_type_bag	*p_bag,
                      			 struct s_gs_connection	*p_connection,
                      			 const char		*name);

int
gs_rl_bag_check_message_mark		(struct s_gs_type_bag	*p_bag,
                            		 struct s_gs_connection	*p_connection,
                            		 const char		*name);

#endif /* GS_RL_TYPE_INTERFACE_H */
