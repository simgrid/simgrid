
#include "msg_simix_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

/** \brief set a configuration variable
 * 
 * Currently existing configuation variable:
 *   - surf_workstation_model (string): Model of workstation to use.  
 *     Possible values (defaults to "KCCFLN05"):
 *     - "CLM03": realistic TCP behavior + basic CPU model (see [CML03 at CCGrid03]) + support for parallel tasks
 *     - "KCCFLN05": realistic TCP behavior + basic CPU model (see [CML03 at CCGrid03]) + failure handling + interference between communications and computations if precised in the platform file.
 * 
 * Example:
 * MSG_config("surf_workstation_model","KCCFLN05");
 */
void MSG_config(const char *name, ...) 
{
  va_list pa;
  /*  xbt_cfg_dump("msg_cfg_set","",_msg_cfg_set);*/
  va_start(pa,name);
	SIMIX_config(name,pa);
  va_end(pa);
	return;
}
