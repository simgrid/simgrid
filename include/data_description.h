/* gs_interface.h */
#ifndef GS_INTERFACE_H
#define GS_INTERFACE_H

/* Mask internal structures to users */
typedef struct s_gs_type_bag		gras_type_bag_t;
typedef struct s_gs_connection		gras_connection_t;
typedef struct s_gs_type		gras_type_t;
typedef struct s_gs_message		gras_message_t;
typedef struct s_gs_net_driver		gras_net_driver_t;
typedef struct s_gs_message_instance 	gras_message_instance_t;
typedef struct s_gs_type_driver		gras_type_driver_t;


/* public functions */
void
gs_init(int    argc,
        char **argv);

void
gs_purge_cmd_line(int   *argc,
                  char **argv);

void
gs_exit(void);

/* -- */

gras_type_t *
gs_type_new_unsigned_integer_elemental(gras_type_bag_t		*p_bag,
                                       gras_connection_t	*p_connection,
                                       const char 		*name,
                                       long int	 		size);

gras_type_t *
gs_type_new_signed_integer_elemental(gras_type_bag_t	*p_bag,
                                     gras_connection_t	*p_connection,
                                     const char 	*name,
                                     long int	 size);

gras_type_t *
gs_type_new_floating_point_elemental(gras_type_bag_t	*p_bag,
                                     gras_connection_t	*p_connection,
                                     const char 	*name,
                                     long int	 size);

gras_type_t *
gs_type_new_struct(gras_type_bag_t	*p_bag,
                   gras_connection_t	*p_connection,
                   const char	*name);

void
gs_type_struct_append_field(gras_type_t 	*p_struct_type,
                            const char		*name,
                            gras_type_t		*p_field_type);

gras_type_t *
gs_type_new_union(gras_type_bag_t	*p_bag,
                  gras_connection_t	*p_connection,
                  const char	*name,

                  int (*field_callback)(void		*vars,
                                        gras_type_t	*p_type,
                                        void		*data));

void
gs_type_union_append_field(gras_type_t	*p_union_type,
                           const char	*name,
                           gras_type_t	*p_field_type);

gras_type_t *
gs_type_new_ref(gras_type_bag_t		*p_bag,
                gras_connection_t	*p_connection,
                const char		*name,
                gras_type_t		*p_referenced_type);

gras_type_t *
gs_type_new_array(gras_type_bag_t	*p_bag,
                  gras_connection_t	*p_connection,
                  const char		*name,
                  long int		 size,
                  gras_type_t		*p_array_element_type);

gras_type_t *
gs_type_new_ignored(gras_type_bag_t	*p_bag,
                    gras_connection_t	*p_connection,
                    const char	*name,
                    long int	 size,
                    long int	 alignment,
                    void	*default_value);

/* -- */

gras_type_t *
gs_type_new_unsigned_integer_elemental_with_callback(gras_type_bag_t	*p_bag,
                                                     gras_connection_t	*p_connection,
                                                     const char	*name,
                                                     long int	 size,

                                                     void (*callback)(void		*vars,
                                                                      gras_type_t	*p_type,
                                                                      void		*data));

gras_type_t *
gs_type_new_signed_integer_elemental_with_callback(gras_type_bag_t	*p_bag,
                                                   gras_connection_t	*p_connection,
                                                   const char		*name,
                                                   long int	 size,

                                                   void (*callback)(void		*vars,
                                                                    gras_type_t	*p_type,
                                                                    void		*data));

gras_type_t *
gs_type_new_floating_point_elemental_with_callback(gras_type_bag_t	*p_bag,
                                                   gras_connection_t	*p_connection,
                                                   const char		*name,
                                                   long int	 size,

                                                   void (*callback)(void		*vars,
                                                                    gras_type_t	*p_type,
                                                                    void		*data));

gras_type_t *
gs_type_new_struct_with_callback(gras_type_bag_t	*p_bag,
                                 gras_connection_t	*p_connection,
                                 const char *name,

                                 void (*before_callback)(void			*vars,
                                                         gras_type_t	*p_type,
                                                         void			*data),

                                 void (*after_callback)(void			*vars,
                                                        gras_type_t	*p_type,
                                                        void			*data));

void
gs_type_struct_append_field_with_callback(gras_type_t	*p_struct_type,
                                          const char			*name,
                                          gras_type_t	*p_field_type,

                                          void (*before_callback)(void			*vars,
                                                                  gras_type_t	*p_type,
                                                                  void			*data),

                                          void (*after_callback)(void			*vars,
                                                                 gras_type_t	*p_type,
                                                                 void			*data));


gras_type_t *
gs_type_new_union_with_callback(gras_type_bag_t	*p_bag,
                                gras_connection_t	*p_connection,
                                const char *name,

                                int (*field_callback)(void		*vars,
                                                       gras_type_t	*p_type,
                                                       void		*data),

                                void (*after_callback)(void		*vars,
                                                       gras_type_t	*p_type,
                                                       void		*data));

void
gs_type_union_append_field_with_callback(gras_type_t	*p_union_type,
                                         const char			*name,
                                         gras_type_t	*p_field_type,

                                         void (*before_callback)(void			*vars,
                                                                 gras_type_t	*p_type,
                                                                 void			*data),

                                         void (*after_callback)(void			*vars,
                                                                gras_type_t	*p_type,
                                                                void			*data));

gras_type_t *
gs_type_new_ref_with_callback(gras_type_bag_t	*p_bag,
                              gras_connection_t	*p_connection,
                              const char		*name,
                              gras_type_t 	*p_referenced_type,

                              int (*type_callback)(void			*vars,
                                                   gras_type_t	*p_type,
                                                   void			*data),

                              void (*after_callback)(void		*vars,
                                                     gras_type_t	*p_type,
                                                     void		*data));

gras_type_t *
gs_type_new_array_with_callback(gras_type_bag_t	*p_bag,
                                gras_connection_t	*p_connection,
                                const char			*name,
                                long int		size,
                                gras_type_t	*p_array_element_type,

                                long int (*size_callback)(void			*vars,
                                                          gras_type_t	*p_type,
                                                          void			*data),

                                void (*after_callback)(void			*vars,
                                                       gras_type_t		*p_type,
                                                       void			*data));

gras_type_t *
gs_type_new_ignored_with_callback(gras_type_bag_t	*p_bag,
                                  gras_connection_t	*p_connection,
                                  const char		*name,
                                  long int	 size,
                                  long int	 alignment,
                                  void		*default_value,
                                  void (*callback)(void			*vars,
                                                   gras_type_t	*p_type,
                                                   void			*data));

/* Automatic parsing of datatypes */
gras_type_t *
_gs_type_parse(gras_type_bag_t	*p_bag,
	       const char	*definition);

#define GRAS_DEFINE_TYPE(name,def) \
  static const char * _gs_this_type_symbol_does_not_exist__##name=#def; def

#define gras_type_symbol_parse(bag,name)                                  \
 _gs_type_parse(bag, _gs_this_type_symbol_does_not_exist__##name)

#define gs_type_get_by_symbol(bag,name)                  \
  (bag->bag_ops->get_type_by_name(bag, NULL, #name) ?    \
     bag->bag_ops->get_type_by_name(bag, NULL, #name) :  \
     gras_type_symbol_parse(bag, name)                   \
  )

/* -- */

void
gs_bootstrap_incoming_connection(gras_type_bag_t	*p_bag,
                                 gras_connection_t	*p_cnx);

void
gs_bootstrap_type_bag(gras_type_bag_t *p_bag);


void
gs_bootstrap_outgoing_connection(gras_type_bag_t	*p_bag,
                                 gras_connection_t	*p_cnx);

/* -- */

gras_message_t *
gs_message_new(gras_type_bag_t	*p_bag,
               gras_connection_t	*p_connection,
               const char			*name);


void
gs_message_append_new_sequence(gras_message_t	*p_message,
                               gras_type_t		*p_type);

gras_message_instance_t *
gs_message_init_send_by_ref(gras_type_bag_t	*p_bag,
                             gras_connection_t	*p_connection,
                             gras_message_t	*p_message);

gras_message_instance_t *
gs_message_init_send_by_name(gras_type_bag_t	*p_bag,
                             gras_connection_t	*p_connection,
                             const char *name);

gras_message_instance_t *
gs_message_init_send_by_code(gras_type_bag_t	*p_bag,
                             gras_connection_t	*p_connection,
                             int code);


void
gs_message_send_next_sequence_ext(void				*vars,
                                  gras_message_instance_t	*p_message_instance,
                                  void				*data);

void
gs_message_send_next_sequence(gras_message_instance_t	*p_message_instance,
                              void				*data);

void *
gs_message_receive_next_sequence(gras_message_instance_t *p_message_instance);

gras_message_instance_t *
gs_message_init_receive(gras_type_bag_t	*p_bag,
                        gras_connection_t	*p_cnx);

/* -- */

void *
gs_vars_alloc(void);

void
gs_vars_free(void *p_vars);

void
gs_vars_enter(void *p_vars);

void
gs_vars_leave(void *p_vars);

void
gs_vars_push(void		*p_vars,
             gras_type_t	*p_type,
             const char		*name,
             void		*data);

void *
gs_vars_get(void		 *p_vars,
            const char		 *name,
            gras_type_t	**pp_type);

void
gs_vars_set(void		*p_vars,
            gras_type_t	*p_type,
            const char		*name,
            void		*data);

void *
gs_vars_pop(void		 *p_vars,
            const char		 *name,
            gras_type_t	**pp_type);

/* -- */

void
gs_net_drivers_init(void);

gras_net_driver_t *
gs_net_driver_init(const char *name);

void
gs_net_driver_exit(gras_net_driver_t	*p_net_driver);

gras_connection_t *
gs_net_connection_connect(gras_net_driver_t	*p_net_driver,
                          void				*arg);

gras_connection_t *
gs_net_connection_accept(gras_net_driver_t	*p_net_driver,
                         void			*arg);

void
gs_net_connection_close(gras_connection_t	*p_connection);

/* -- */

void
gs_type_drivers_init(void);

gras_type_driver_t *
gs_type_driver_init(const char *name);

void
gs_type_driver_exit(gras_type_driver_t	*p_type_driver);


gras_type_bag_t *
gs_type_bag_alloc(gras_type_driver_t	*p_driver);

void
gs_type_bag_free(gras_type_bag_t	*p_bag);

/* -- */

void *
gs_memdup(const void * const ptr,
          const size_t       length);


#endif /* GS_INTERFACE_H */
