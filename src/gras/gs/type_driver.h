/* gs_type_driver.h */
#ifndef GS_TYPE_DRIVER_H
#define GS_TYPE_DRIVER_H

/* used structs */
struct s_gs_type_bag;
struct s_gs_type_driver;

struct s_gs_type_driver_ops {

        void
        (*_init)	(struct s_gs_type_driver	*p_type_driver);

        void
        (*_exit)	(struct s_gs_type_driver	*p_type_driver);
};

struct s_gs_type_driver {
        struct s_gs_type_driver_ops	*type_ops;
        struct s_gs_type_bag_ops	*bag_ops;
        void				*specific;
};


#endif /* GS_TYPE_DRIVER_H */
