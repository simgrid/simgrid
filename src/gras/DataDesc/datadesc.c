/* gs.c */

#include "DataDesc/gs_private.h"

GRAS_LOG_NEW_DEFAULT_CATEGORY(NDR);

void
gs_init(int    argc,
        char **argv) {

        (void)argc;
        (void)argv;
	
	DEBUG0("Init");
        gs_net_drivers_init();
        gs_type_drivers_init();
}

void
gs_purge_cmd_line(int   *argc,
                  char **argv) {
        (void)argc;
        (void)argv;

        /**/
}

void
gs_exit() {
        /**/
}
