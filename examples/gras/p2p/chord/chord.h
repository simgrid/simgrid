/* 
 * vim:ts=2:sw=2:noexpandtab
 */

#include <gras.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(chord,"Messages specific to this example");

typedef enum msg_typus{
	PING,
	PONG,
	GET_PRE,
	REP_PRE,
	GET_SUC,
	REP_SUC,
	STD,
}msg_typus;

/*GRAS_DEFINE_TYPE(s_pbio,
	struct s_pbio{
		msg_typus type;
		int dest;
		char msg[1024];
	};
);
typedef struct s_pbio pbio_t;*/

GRAS_DEFINE_TYPE(s_ping,
	struct s_ping{
		int id;
	};
);
typedef struct s_ping ping_t;

GRAS_DEFINE_TYPE(s_pong,
	struct s_pong{
		int id;
		int failed;
	};
);
typedef struct s_pong pong_t;

GRAS_DEFINE_TYPE(s_get_suc,
	struct s_get_suc{
		int id;
	};
);
typedef struct s_get_suc get_suc_t;

GRAS_DEFINE_TYPE(s_rep_suc,
	struct s_rep_suc{
		int id;
		char host[1024];
		int port;
	};
);
typedef struct s_rep_suc rep_suc_t;

typedef struct finger_elem{
	int id;
	char host[1024];
	int port;
}finger_elem;
