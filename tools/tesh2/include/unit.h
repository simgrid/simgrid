#ifndef __UNIT_H	
#define __UNIT_H

#include <com.h>

#ifdef __cplusplus
extern "C" {
#endif

unit_t
unit_new(runner_t runner, suite_t owner, fstream_t fstream);

int
unit_free(void** unitptr);


int
unit_reset(unit_t unit);

void
unit_run(unit_t unit, xbt_os_mutex_t mutex);

void
unit_interrupt(unit_t unit);

void
unit_verbose(unit_t unit);

void
unit_handle_failure(unit_t unit);

void 
unit_handle_line(unit_t unit, context_t context, xbt_os_mutex_t mutex, const char * filepos, char *line);

void 
unit_pushline(unit_t unit, context_t context, xbt_os_mutex_t mutex, const char* filepos, char kind, char *line);

void
unit_handle_include(unit_t unit, context_t context, xbt_os_mutex_t mutex, const char* file_name);

void
unit_parse(unit_t unit, context_t context, xbt_os_mutex_t mutex, const char* file_name, FILE* stream);

void
unit_handle_suite(unit_t unit, context_t context, xbt_os_mutex_t mutex, const char* description);

#ifdef __cplusplus
}
#endif


#endif /* !__UNIT_H */
