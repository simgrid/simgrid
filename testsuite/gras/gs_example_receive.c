/* gs_example_receive.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gras.h>

/* structs */
struct list {
        int          v;
        struct list *l;
};

struct s_pair {
        int *pa;
        int *pb;
};

struct s_mixed {
        unsigned char c1;
        unsigned long int l1;
        unsigned char c2;
        unsigned long int l2;
};

/* prototypes */
void disp_l(struct list *l);
long int
string_size_callback(void		*vars,
                     struct s_gs_type	*p_type,
                     void		*data);

/* local functions */
void
disp_l(struct list *l) {
        if (l) {
                printf("%d ", l->v);
                disp_l(l->l);
        }
}

long int
string_size_callback(void		*vars,
                     struct s_gs_type	*p_type,
                     void		*data) {
        return 1+(long int)strlen(data);
}

/* main */
int
main(int argc, char **argv) {
        struct list			**pl		= NULL;
        int 				 *array		= NULL;
        struct s_pair                    *p_pair        = NULL;
        char				 *str		= NULL;
	//        struct s_mixed                   *mixed         = NULL;

        struct s_gs_type_driver		 *t_driver      = NULL;
        struct s_gs_net_driver		 *n_driver      = NULL;
        struct s_gs_type_bag		 *bag           = NULL;
        struct s_gs_connection		 *cnx           = NULL;

        struct s_gs_type_bag_ops	 *bag_ops       = NULL;

        struct s_gs_type		 *t_signed_int	= NULL;
        struct s_gs_type		 *t_list	= NULL;
        struct s_gs_type		 *t_ref_list	= NULL;

        struct s_gs_type		 *t_array	= NULL;

        struct s_gs_type		 *t_ref_sint	= NULL;
        struct s_gs_type		 *t_pair	= NULL;

        struct s_gs_type                 *t_char        = NULL;
        struct s_gs_type                 *t_string      = NULL;

        struct s_gs_type		*t_u_char     = NULL;
        struct s_gs_type		*t_u_long_int = NULL;
        struct s_gs_type		*t_mixed      = NULL;

        struct s_gs_message_instance	 *mi		= NULL;

        gs_init(argc, argv);
        gs_purge_cmd_line(&argc, argv);

        t_driver = gs_type_driver_init("rl");
        n_driver = gs_net_driver_init("fd");

        bag	= gs_type_bag_alloc(t_driver);
        bag_ops = bag->bag_ops;

        {
                int fd = 0;
                cnx = gs_net_connection_accept(n_driver, &fd);
        }

        bag_ops->register_incoming_connection(bag, cnx);

        /* sequence 1 */
        t_signed_int	= bag_ops->get_type_by_name(bag, NULL, "signed int");
        t_list		= gs_type_new_struct(bag, NULL, "list");
        t_ref_list	= gs_type_new_ref(bag, NULL, "p_list", t_list);

        gs_type_struct_append_field(t_list, "v", t_signed_int);
        gs_type_struct_append_field(t_list, "l", t_ref_list);

        /* sequence 2 */
        t_array	= gs_type_new_array(bag, NULL, "array", 5, t_signed_int);

        /* sequence 3 */
        t_ref_sint	= gs_type_new_ref(bag, NULL, "p_ref_sint", t_signed_int);
        t_pair		= gs_type_new_struct(bag, NULL, "pair");

        gs_type_struct_append_field(t_pair, "pa", t_ref_sint);
        gs_type_struct_append_field(t_pair, "pb", t_ref_sint);

        /* sequence 4 */
        t_char	= bag_ops->get_type_by_name(bag, NULL, "signed char");
        t_string = gs_type_new_array_with_callback(bag, NULL, "string", -1, t_char, string_size_callback, NULL);

        /* sequence 5 */
        t_u_char	= bag_ops->get_type_by_name(bag, NULL, "unsigned char");
        t_u_long_int	= bag_ops->get_type_by_name(bag, NULL, "unsigned long int");
        t_mixed		= gs_type_new_struct(bag, NULL, "mixed");
   
        gs_type_struct_append_field(t_mixed, "c1", t_u_char);
        gs_type_struct_append_field(t_mixed, "l1", t_u_long_int);
        gs_type_struct_append_field(t_mixed, "c2", t_u_char);
        gs_type_struct_append_field(t_mixed, "l2", t_u_long_int);
	
        /* message receive */
        mi	= gs_message_init_receive(bag, cnx);
        fprintf(stderr, "\nreceiving sequence 1\n----------------\n");
        pl	= gs_message_receive_next_sequence(mi);

        printf("( ");
        disp_l(*pl);
        printf(")\n");

        fprintf(stderr, "\nreceiving sequence 2\n----------------\n");
        array	= gs_message_receive_next_sequence(mi);

        {
                int i = 0;
                for (i = 0; i < 5; i++) {
                        printf("array[%d] = %d\n", i, array[i]);
                }
        }

        fprintf(stderr, "\nreceiving sequence 3\n----------------\n");
        p_pair	= gs_message_receive_next_sequence(mi);
        printf("pair.pa = %p, *pair.pa = %d\n", (*p_pair).pa, *((*p_pair).pa));
        printf("pair.pb = %p, *pair.pb = %d\n", (*p_pair).pb, *((*p_pair).pb));

        fprintf(stderr, "\nreceiving sequence 4\n----------------\n");
        str = gs_message_receive_next_sequence(mi);
        printf("str = %s\n", str);

	/*
        fprintf(stderr, "\nreceiving sequence 5\n----------------\n");
        mixed = gs_message_receive_next_sequence(mi);
        printf("c1=%c c2=%c; l1=%ld l2=%ld\n", 
	       mixed->c1, mixed->c2, mixed->l1, mixed->l2);
	*/
        gs_exit();

        return 0;
}
