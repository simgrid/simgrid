/* gs_type.h */
#ifndef GS_TYPE_H
#define GS_TYPE_H


/*
 * Set of categories
 */
union u_gs_category {
        void                             *undefined_data;
        struct s_gs_cat_elemental        *elemental_data;
        struct s_gs_cat_struct           *struct_data;
        struct s_gs_cat_union            *union_data;
        struct s_gs_cat_ref              *ref_data;
        struct s_gs_cat_array            *array_data;
        struct s_gs_cat_ignored          *ignored_data;
};


/*
 * A type
 */
struct s_gs_type {
        int			 	  code;
        char			 	 *name;

        long int		  	  size;

        long int                          alignment;
        long int		  	  aligned_size;

        enum  e_gs_type_category          category_code;
        union u_gs_category               category;

        void				 (*before_callback)(void		*vars,
                                                            struct s_gs_type	*p_type,
                                                            void		*data);

        void				 (*after_callback)(void			*vars,
                                                           struct s_gs_type	*p_type,
                                                           void			*data);
};

#endif /* GS_TYPE_H */
