/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_private.h"
#include "xbt/dict.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(network, surf ,"Logging specific to the SURF network module");

surf_network_resource_t surf_network_resource = NULL;

static xbt_dict_t network_set = NULL;


static void *route_new(const char *src, const char* dst)
{
  return NULL;
}

/*  
   Semantic:  name       scale     initial     power     initial     state
                                    power      trace      state      trace
   
   Token:   TOKEN_WORD TOKEN_WORD TOKEN_WORD TOKEN_WORD TOKEN_WORD TOKEN_WORD
   Type:     string      float      float      string     ON/OFF     string
*/

static void parse_route(void)
{
  route_new(NULL,NULL);
}

static void parse_file(const char *file)
{
  e_surf_token_t token;

  find_section(file,"NETWORK");

  while(1) {
    token=surf_parse();

    if(token==TOKEN_END_SECTION) break;
    if(token==TOKEN_NEWLINE) continue;
    
    if(token==TOKEN_WORD) parse_route();
    else {CRITICAL1("Parse error line %d\n",line_pos);xbt_abort();}
  }

  close_section("NETWORK");  
}

static void *name_service(const char *name)
{
  return NULL;
}

static const char *get_resource_name(void *resource_id)
{
  return NULL;
}

static int resource_used(void *resource_id)
{
  return 0;
}

static void action_free(surf_action_t action)
{
  return;
}

static void action_cancel(surf_action_t action)
{
  return;
}

static void action_recycle(surf_action_t action)
{
  return;
}

static void action_change_state(surf_action_t action, e_surf_action_state_t state)
{
  return;
}

static xbt_heap_float_t share_resources(xbt_heap_float_t now)
{
  return -1.0;
}

static void update_actions_state(xbt_heap_float_t now,
				 xbt_heap_float_t delta)
{
  return;
}

static void update_resource_state(void *id,
				  tmgr_trace_event_t event_type, 
				  xbt_maxmin_float_t value)
{
  return;
}

static surf_action_t communicate(void *src, void *dst, xbt_maxmin_float_t size)
{
  return NULL;
}

static void finalize(void)
{
}

static void surf_network_resource_init_internal(void)
{
  s_surf_action_t action;

  surf_network_resource = xbt_new0(s_surf_network_resource_t,1);
  
  surf_network_resource->common_private = xbt_new0(s_surf_resource_private_t,1);
  surf_network_resource->common_public = xbt_new0(s_surf_resource_public_t,1);
/*   surf_network_resource->extension_private = xbt_new0(s_surf_network_resource_extension_private_t,1); */
  surf_network_resource->extension_public = xbt_new0(s_surf_network_resource_extension_public_t,1);

  surf_network_resource->common_public->states.ready_action_set=
    xbt_swag_new(xbt_swag_offset(action,state_hookup));
  surf_network_resource->common_public->states.running_action_set=
    xbt_swag_new(xbt_swag_offset(action,state_hookup));
  surf_network_resource->common_public->states.failed_action_set=
    xbt_swag_new(xbt_swag_offset(action,state_hookup));
  surf_network_resource->common_public->states.done_action_set=
    xbt_swag_new(xbt_swag_offset(action,state_hookup));

  surf_network_resource->common_public->name_service = name_service;
  surf_network_resource->common_public->get_resource_name = get_resource_name;
  surf_network_resource->common_public->resource_used = resource_used;
  surf_network_resource->common_public->action_get_state=surf_action_get_state;
  surf_network_resource->common_public->action_free = action_free;
  surf_network_resource->common_public->action_cancel = action_cancel;
  surf_network_resource->common_public->action_recycle = action_recycle;
  surf_network_resource->common_public->action_change_state = action_change_state;

  surf_network_resource->common_private->share_resources = share_resources;
  surf_network_resource->common_private->update_actions_state = update_actions_state;
  surf_network_resource->common_private->update_resource_state = update_resource_state;
  surf_network_resource->common_private->finalize = finalize;

  surf_network_resource->extension_public->communicate = communicate;

  network_set = xbt_dict_new();

  xbt_assert0(maxmin_system,"surf_init has to be called first!");
}

void surf_network_resource_init(const char* filename)
{
  surf_network_resource_init_internal();
  parse_file(filename);
  xbt_dynar_push(resource_list, &surf_network_resource);
}
