/* gs_categories.h */
#ifndef GS_CATEGORIES_H
#define GS_CATEGORIES_H

/*
 * Avoid some strange warnings with callback defs.
 */
struct s_gs_type;

/*
 * categories of types
 */
enum e_gs_type_category {
        e_gs_type_cat_undefined = 0,

        e_gs_type_cat_elemental,
        e_gs_type_cat_struct,
        e_gs_type_cat_union,
        e_gs_type_cat_ref,
        e_gs_type_cat_array,
        e_gs_type_cat_ignored,

        e_gs_type_cat_invalid
};


/*
 * Elemental category
 */
enum e_gs_elemental_encoding {
        e_gs_elemental_encoding_undefined = 0,

        e_gs_elemental_encoding_unsigned_integer,
        e_gs_elemental_encoding_signed_integer,
        e_gs_elemental_encoding_floating_point,

        e_gs_elemental_encoding_invalid
};

struct s_gs_cat_elemental {
        int				encoding;
};


/*
 * Structure category
 */
struct s_gs_cat_struct_field {

        char 			       *name;
        long int		        offset;
        int                             code;

        void				(*before_callback)(void			*vars,
                                                           struct s_gs_type	*p_type,
                                                           void			*data);

        void				(*after_callback)(void			*vars,
                                                          struct s_gs_type	*p_type,
                                                          void			*data);
};

struct s_gs_cat_struct {

        int			  	  nb_fields;
        struct s_gs_cat_struct_field	**field_array;
};


/*
 * Union category
 */
struct s_gs_cat_union_field {

        char 			       *name;
        int                             code;

        void				(*before_callback)(void			*vars,
                                                           struct s_gs_type	*p_type,
                                                           void			*data);

        void				(*after_callback)(void			*vars,
                                                          struct s_gs_type	*p_type,
                                                          void			*data);
};

struct s_gs_cat_union {

        int			  	  nb_fields;
        struct s_gs_cat_union_field	**field_array;

        /* callback used to return the field number  */
        int                         (*callback)(void		 *vars,
                                                struct s_gs_type *p_type,
                                                void		 *data);
};


/*
 * Ref category
 */
struct s_gs_cat_ref {
        int	 	 		code;

        /* callback used to return the referenced type number  */
        int                         (*callback)(void		 *vars,
                                                struct s_gs_type *p_type,
                                                void		 *data);
};


/*
 * Array_categorie
 */
struct s_gs_cat_array {
        int	 	 		code;

        /* element_count < 0 means dynamically defined */
        long int                  	  element_count;

        /* callback used to return the dynamic length */
        long int                         (*callback)(void		*vars,
                                                     struct s_gs_type	*p_type,
                                                     void		*data);
};

/*
 * Ignored category
 */
struct s_gs_cat_ignored {
        void	 	 		*default_value;
};

gras_type_t *
gs_type_new_elemental(gras_type_bag_t	*p_bag,
                      gras_connection_t	*p_connection,
                      const char 	*name,
                      enum e_gs_elemental_encoding	 encoding,
                      long int 				 size);

gras_type_t *
gs_type_new_elemental_with_callback(gras_type_bag_t	*p_bag,
                                    gras_connection_t	*p_connection,
                                    const char				*name,
                                    enum e_gs_elemental_encoding	 encoding,
                                    long int				 size,

                                    void (*callback)(void		*vars,
                                                     gras_type_t	*p_type,
                                                     void		*data));



#endif /* GS_CATEGORIES_H */
