
#include "msg_simix_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"


/** \ingroup msg_easier_life
 * \brief An application deployer.
 *
 * Creates the process described in \a file.
 * \param file a filename of a xml description of the application. This file 
 * follows this DTD :
 *
 *     \include surfxml.dtd
 *
 * Here is a small example of such a platform 
 *
 *     \include small_deployment.xml
 *
 * Have a look in the directory examples/msg/ to have a bigger example.
 */
void MSG_launch_application(const char *file) 
{
  xbt_assert0(msg_global,"MSG_global_init_args has to be called before MSG_launch_application.");
	SIMIX_launch_application(file);

	return;
}

/** \ingroup msg_easier_life
 * \brief Registers a #m_process_code_t code in a global table.
 *
 * Registers a code function in a global table. 
 * This table is then used by #MSG_launch_application. 
 * \param name the reference name of the function.
 * \param code the function
 */
void MSG_function_register(const char *name,m_process_code_t code)
{
	SIMIX_function_register(name, code);
	return;
}

/** \ingroup msg_easier_life
 * \brief Registers a #m_process_t code in a global table.
 *
 * Registers a code function in a global table. 
 * This table is then used by #MSG_launch_application. 
 * \param name the reference name of the function.
 */
m_process_code_t MSG_get_registered_function(const char *name)
{
  m_process_code_t code = NULL;

	code = (m_process_code_t)SIMIX_get_registered_function(name);

  return code;
}

