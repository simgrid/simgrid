#ifndef _XBT_CONTEXT_FACTORY_H	
#define _XBT_CONTEXT_FACTORY_H

#include "portable.h"
#include "xbt/function_types.h"
#include "xbt_context_private.h"

SG_BEGIN_DECL()

/* forward declaration */
struct s_xbt_context_factory;

/* this function describes the interface that all context factory must implement */
typedef xbt_context_t (*xbt_pfn_context_factory_create_context_t)(const char*, xbt_main_func_t, void_f_pvoid_t, void*, void_f_pvoid_t, void*, int, char**);
typedef int (*xbt_pfn_context_factory_create_maestro_context_t)(xbt_context_t*);

/* this function finalize the specified context factory */
typedef int (*xbt_pfn_context_factory_finalize_t)(struct s_xbt_context_factory**);

/* this interface is used by the xbt context module to create the appropriate concept */
typedef struct s_xbt_context_factory
{
	xbt_pfn_context_factory_create_maestro_context_t create_maestro_context;	/* create the context of the maestro	*/
	xbt_pfn_context_factory_create_context_t create_context;					/* create a new context					*/
	xbt_pfn_context_factory_finalize_t finalize;								/* finalize the context factory			*/
	const char* name;															/* the name of the context factory		*/
	
}s_xbt_context_factory_t;

/**
 * This function select a context factory associated with the name specified by
 * the parameter name.
 * If successful the function returns 0. Otherwise the function returns the error
 * code.
 */
int
xbt_context_select_factory(const char* name);

/**
 * This function initialize a context factory from the name specified by the parameter
 * name.
 * If successful the factory specified by the parameter factory is initialized and the
 * function returns 0. Otherwise the function returns the error code.
 */
int
xbt_context_init_factory_by_name(xbt_context_factory_t* factory, const char* name);


SG_END_DECL()

#endif /* !_XBT_CONTEXT_FACTORY_H */
