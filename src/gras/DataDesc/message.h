/* gs_message.h */
#ifndef GS_MESSAGE_H
#define GS_MESSAGE_H

struct s_gs_message {
        int                   code;
        char                 *name;

        struct s_gs_sequence *first;
        struct s_gs_sequence *last;
};

struct s_gs_message_instance {
        struct s_gs_message 	*p_message;
        struct s_gs_sequence    *current;
        struct s_gs_type_bag	*p_bag;
        struct s_gs_connection	*p_connection;
};

#endif /* GS_MESSAGE_H */
