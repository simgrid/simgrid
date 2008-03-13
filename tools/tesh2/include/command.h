#ifndef __COMMAND_H
#define __COMMAND_H

#include <com.h>

#ifdef __cplusplus
extern "C" {
#endif

command_t
command_new(unit_t unit, context_t context, xbt_os_mutex_t mutex);

void
command_free(command_t* command);

void
command_run(command_t command);

void
command_exec(command_t command, const char* command_line);

void
command_wait(command_t command);

void
command_interrupt(command_t command);

void
command_display_status(command_t command);

void
command_handle_failure(command_t command, cs_reason_t reason);

void command_kill(command_t command);

void
command_check(command_t command);

#ifdef __cplusplus
}
#endif

#endif /* !_COMMAND_H */
