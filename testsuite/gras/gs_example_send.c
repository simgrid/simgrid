/* gs_example_send */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gras.h>

//#define PARSING

/* structs */
struct list {
        int          v;
        struct list *l;
};

struct s_pair {
        int *pa;
        int *pb;
};

#ifdef PARSING
GRAS_DEFINE_TYPE(struct_s_mixed,
  struct s_mixed {
        unsigned char c1;
        unsigned long int l1;
        unsigned char c2;
        unsigned long int l2;
 }
);
#else
  struct s_mixed {
        unsigned char c1;
        unsigned long int l1;
        unsigned char c2;
        unsigned long int l2;
 };
#endif

/* prototypes */
struct list * cons(int v, struct list *l);
long int
string_size_callback(void		*vars,
                     struct s_gs_type	*p_type,
                     void		*data);

/* local functions */
struct list *
cons(int v, struct list *l) {
        struct list *nl = malloc(sizeof (struct list));

        nl->v = v;
        nl->l = l;

        return nl;
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
        struct list                  *l            = NULL;

        struct s_pair                 pair;

        int array[5] =
                {
                        11, 12, 13, 14, 15,
                };
   
        struct s_mixed                mixed =
                {
	                'a',1.0,'b',2.0
		};

        struct s_gs_type_driver		*t_driver     = NULL;
        struct s_gs_net_driver		*n_driver     = NULL;
        struct s_gs_type_bag		*bag          = NULL;
        struct s_gs_connection		*cnx          = NULL;

        struct s_gs_type_bag_ops	*bag_ops      = NULL;

        struct s_gs_type		*t_signed_int = NULL;
        struct s_gs_type		*t_list       = NULL;
        struct s_gs_type		*t_ref_list   = NULL;

        struct s_gs_type		*t_array      = NULL;

        struct s_gs_type		*t_ref_sint   = NULL;
        struct s_gs_type		*t_pair       = NULL;

        struct s_gs_type		*t_char       = NULL;
        struct s_gs_type		*t_string     = NULL;

#ifndef PARSING
        struct s_gs_type		*t_u_char     = NULL;
        struct s_gs_type		*t_u_long_int = NULL;
#endif

        struct s_gs_type		*t_mixed      = NULL;

        struct s_gs_message		*m            = NULL;
        struct s_gs_message_instance	*mi           = NULL;


        gs_init(argc, argv);
        gs_purge_cmd_line(&argc, argv);
	gras_log_control_set("NDR.thresh=debug");

        t_driver = gs_type_driver_init("rl");
        n_driver = gs_net_driver_init("fd");

        bag	= gs_type_bag_alloc(t_driver);
        bag_ops = bag->bag_ops;

        {
                int fd = 1;
                cnx = gs_net_connection_connect(n_driver, &fd);
        }

        bag_ops->register_outgoing_connection(bag, cnx);

        /* sequence 1 */
        t_signed_int	= bag_ops->get_type_by_name(bag, NULL, "signed int");
        t_list		= gs_type_new_struct(bag, NULL, "list");
        t_ref_list	= gs_type_new_ref(bag, NULL, "p_list", t_list);

        gs_type_struct_append_field(t_list, "v", t_signed_int);
        gs_type_struct_append_field(t_list, "l", t_ref_list);

        /* sequence 2 */
        t_array		= gs_type_new_array(bag, NULL, "array", 5, t_signed_int);

        /* sequence 3 */
        t_ref_sint	= gs_type_new_ref(bag, NULL, "p_ref_sint", t_signed_int);
        t_pair		= gs_type_new_struct(bag, NULL, "pair");

        gs_type_struct_append_field(t_pair, "pa", t_ref_sint);
        gs_type_struct_append_field(t_pair, "pb", t_ref_sint);

        /* sequence 4 */
        t_char	= bag_ops->get_type_by_name(bag, NULL, "signed char");
        t_string = gs_type_new_array_with_callback(bag, NULL, "string", -1, t_char, string_size_callback, NULL);

        /* sequence 5 */
#ifdef PARSING
        t_mixed		= gs_type_get_by_symbol(bag,struct_s_mixed);
#else
        t_u_char	= bag_ops->get_type_by_name(bag, NULL, "unsigned char");
        t_u_long_int	= bag_ops->get_type_by_name(bag, NULL, "unsigned long int");
        t_mixed		= gs_type_new_struct(bag, NULL, "s_mixed");
   
        gs_type_struct_append_field(t_mixed, "c1", t_u_char);
        gs_type_struct_append_field(t_mixed, "l1", t_u_long_int);
        gs_type_struct_append_field(t_mixed, "c2", t_u_char);
        gs_type_struct_append_field(t_mixed, "l2", t_u_long_int);
#endif

        /* message declaration */
        m = gs_message_new(bag, NULL, "my msg");
	gs_message_append_new_sequence(m, t_ref_list);
        gs_message_append_new_sequence(m, t_array);
        gs_message_append_new_sequence(m, t_pair);
        gs_message_append_new_sequence(m, t_string);
        gs_message_append_new_sequence(m, t_mixed);

        /* data setup */
        l = cons (1, l);
        l = cons (2, l);
        l = cons (3, l);

        pair.pa = malloc(sizeof(int));
        pair.pb = pair.pa;
        *(pair.pa) = 17;

        /* message send */
        mi = gs_message_init_send_by_name(bag, cnx, "my msg");
        fprintf(stderr, "\nsending sequence 1\n----------------\n");
        gs_message_send_next_sequence(mi, &l);
        fprintf(stderr, "\nsending sequence 2\n----------------\n");
        gs_message_send_next_sequence(mi, (void*)array);
        fprintf(stderr, "\nsending sequence 3\n----------------\n");
        gs_message_send_next_sequence(mi, &pair);
        fprintf(stderr, "\nsending sequence 4\n----------------\n");
        gs_message_send_next_sequence(mi, (void*)"Hello, World");
        fprintf(stderr, "\nsending sequence 5\n----------------\n");
        gs_message_send_next_sequence(mi, (void*)&mixed);

        gs_exit();

        return 0;
}
