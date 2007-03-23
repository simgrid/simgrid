
#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

/** \defgroup msg_easier_life      Platform and Application management
 *  \brief This section describes functions to manage the platform creation
 *  and the application deployment. You should also have a look at 
 *  \ref MSG_examples  to have an overview of their usage.
 *    \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Platforms and Applications" --> \endhtmlonly
 * 
 */

/********************************* MSG **************************************/

/** \ingroup msg_easier_life
 * \brief A name directory service...
 *
 * Finds a m_host_t using its name.
 * \param name the name of an host.
 * \return the corresponding host
 */
m_host_t MSG_get_host_by_name(const char *name)
{
	smx_host_t simix_h = NULL;

	simix_h = SIMIX_host_get_by_name(name);
	if (simix_h == NULL) {
		return NULL;
	}
	else return (m_host_t)simix_h->data;
}

/** \ingroup msg_easier_life
 * \brief A platform constructor.
 *
 * Creates a new platform, including hosts, links and the
 * routing_table. 
 * \param file a filename of a xml description of a platform. This file 
 * follows this DTD :
 *
 *     \include surfxml.dtd
 *
 * Here is a small example of such a platform 
 *
 *     \include small_platform.xml
 *
 * Have a look in the directory examples/msg/ to have a big example.
 */
void MSG_create_environment(const char *file) 
{
  smx_host_t *workstation = NULL;
	int i;

	SIMIX_create_environment(file);

	/* Initialize MSG hosts */
	workstation = SIMIX_host_get_table();
	for (i=0; i< SIMIX_host_get_number();i++) {
		__MSG_host_create(workstation[i], NULL);
	}
  return;
}

