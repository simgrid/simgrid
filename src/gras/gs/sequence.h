/* gs_sequence.h */
#ifndef GS_SEQUENCE_H
#define GS_SEQUENCE_H

struct s_gs_sequence {
        struct s_gs_message	*message;
        int                   	 code;
        struct s_gs_sequence	*next;
};

#endif /* GS_SEQUENCE_H */
