
#include "private.h"
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


  return code;
}

