/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_private.h"
#include "xbt/dict.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(network, surf,
				"Logging specific to the SURF network module");

surf_network_resource_t surf_network_resource = NULL;

static xbt_dict_t network_link_set = NULL;
static xbt_dict_t network_card_set = NULL;

static int card_number = 0;
static network_link_t **routing_table = NULL;
static int *routing_table_size = NULL;

#define ROUTE(i,j) routing_table[(i)+(j)*card_number]
#define ROUTE_SIZE(i,j) routing_table_size[(i)+(j)*card_number]

static void create_routing_table(void)
{
  routing_table = xbt_new0(network_link_t *,card_number*card_number);
  routing_table_size = xbt_new0(int,card_number*card_number);
}

static void network_link_free(void *nw_link)
{
  xbt_free(nw_link);
}

static network_link_t network_link_new(const char *name,
				       xbt_maxmin_float_t bw_initial,
				       tmgr_trace_t bw_trace,
				       xbt_maxmin_float_t lat_initial,
				       tmgr_trace_t lat_trace,
				       e_surf_network_link_state_t state_initial,
				       tmgr_trace_t state_trace)
{
  network_link_t nw_link = xbt_new0(s_network_link_t, 1);

  
  nw_link->resource = (surf_resource_t) surf_network_resource;
  nw_link->name = name;
  nw_link->bw_current = bw_initial;
  if (bw_trace)
    nw_link->bw_event =
	tmgr_history_add_trace(history, bw_trace, 0.0, 0, nw_link);
  nw_link->lat_current = lat_initial;
  if (lat_trace)
    nw_link->lat_event =
	tmgr_history_add_trace(history, lat_trace, 0.0, 0, nw_link);
  nw_link->state_current = state_initial;
  if (state_trace)
    nw_link->state_event =
	tmgr_history_add_trace(history, state_trace, 0.0, 0, nw_link);

  nw_link->constraint =
      lmm_constraint_new(maxmin_system, nw_link,
			 nw_link->bw_current);

  xbt_dict_set(network_link_set, name, nw_link, network_link_free);

  return nw_link;
}

static int network_card_new(const char *card_name)
{
  network_card_t card = NULL;

  xbt_dict_get(network_card_set, card_name, (void *) &card);

  if(!card) {
    card = xbt_new0(s_network_card_t,1);
    card->name=xbt_strdup(card_name);
    card->id=card_number++;
    xbt_dict_set(network_card_set, card_name, card, NULL);
  } 
  return card->id;
}

static void route_new(int src_id, int dst_id,
		      char* *links, int nb_link)
{
  network_link_t *link_list = NULL;
  int i ; 

  ROUTE_SIZE(src_id,dst_id) = nb_link;
  link_list = (ROUTE(src_id,dst_id) = xbt_new0(network_link_t,nb_link));
  for(i=0; i < nb_link; i++) {
    xbt_dict_get(network_link_set,links[i], (void *) &(link_list[i]));
    xbt_free(links[i]);
  }
  xbt_free(links);  
}

/*  
   Semantic:  name       initial   bandwidth   initial   latency    initial     state
                        bandwidth    trace     latency    trace      state      trace
   
   Token:   TOKEN_WORD TOKEN_WORD TOKEN_WORD TOKEN_WORD TOKEN_WORD TOKEN_WORD TOKEN_WORD
   Type:     string       float      string    float      string     ON/OFF     string
*/
static void parse_network_link(void)
{
  e_surf_token_t token;
  char *name;
  xbt_maxmin_float_t bw_initial;
  tmgr_trace_t bw_trace;
  xbt_maxmin_float_t lat_initial;
  tmgr_trace_t lat_trace;
  e_surf_network_link_state_t state_initial;
  tmgr_trace_t state_trace;

  name = xbt_strdup(surf_parse_text);

  surf_parse_float(&bw_initial);
  surf_parse_trace(&bw_trace);
  surf_parse_float(&lat_initial);
  surf_parse_trace(&lat_trace);

  token = surf_parse();		/* state_initial */
  xbt_assert1((token == TOKEN_WORD), "Parse error line %d", line_pos);
  if (strcmp(surf_parse_text, "ON") == 0)
    state_initial = SURF_NETWORK_LINK_ON;
  else if (strcmp(surf_parse_text, "OFF") == 0)
    state_initial = SURF_NETWORK_LINK_OFF;
  else {
    CRITICAL2("Invalid cpu state (line %d): %s neq ON or OFF\n", line_pos,
	      surf_parse_text);
    xbt_abort();
  }

  surf_parse_trace(&state_trace);

  network_link_new(name, bw_initial, bw_trace, 
		   lat_initial, lat_trace,
		   state_initial, state_trace);  
}

/*  
   Semantic:  source   destination           network
                                              links
   
   Token:   TOKEN_WORD TOKEN_WORD TOKEN_LP TOKEN_WORD* TOKEN_RP
   Type:     string       string             string
*/
static void parse_route(int fake)
{
  int src_id = -1;
  int dst_id = -1;
  int nb_link = 0;
  char **link_name = NULL;
  e_surf_token_t token;

  src_id = network_card_new(surf_parse_text);

  token = surf_parse();
  xbt_assert1((token == TOKEN_WORD), "Parse error line %d", line_pos);  
  dst_id = network_card_new(surf_parse_text);

  token = surf_parse();
  xbt_assert1((token == TOKEN_LP), "Parse error line %d", line_pos);  
  
  while((token = surf_parse())==TOKEN_WORD) {
    if(!fake) {
      nb_link++;
      link_name=xbt_realloc(link_name, (nb_link) * sizeof(char*));
      link_name[(nb_link)-1]=xbt_strdup(surf_parse_text);
    }
  }
  xbt_assert1((token == TOKEN_RP), "Parse error line %d", line_pos);  

  if(!fake) route_new(src_id,dst_id,link_name, nb_link);
}

static void parse_file(const char *file)
{
  e_surf_token_t token;

  /* Figuring out the network links in the system */
  find_section(file, "NETWORK");
  while (1) {
    token = surf_parse();

    if (token == TOKEN_END_SECTION)
      break;
    if (token == TOKEN_NEWLINE)
      continue;

    if (token == TOKEN_WORD)
      parse_network_link();
    else {
      CRITICAL1("Parse error line %d\n", line_pos);
      xbt_abort();
    }
  }
  close_section("NETWORK");

  /* Figuring out the network cards used */
  find_section(file, "ROUTE");
  while (1) {
    token = surf_parse();

    if (token == TOKEN_END_SECTION)
      break;
    if (token == TOKEN_NEWLINE)
      continue;

    if (token == TOKEN_WORD)
      parse_route(1);
    else {
      CRITICAL1("Parse error line %d\n", line_pos);
      xbt_abort();
    }
  }
  close_section("ROUTE");

  create_routing_table();

  /* Building the routes */
  find_section(file, "ROUTE");
  while (1) {
    token = surf_parse();

    if (token == TOKEN_END_SECTION)
      break;
    if (token == TOKEN_NEWLINE)
      continue;

    if (token == TOKEN_WORD)
      parse_route(0);
    else {
      CRITICAL1("Parse error line %d\n", line_pos);
      xbt_abort();
    }
  }
  close_section("ROUTE");
}

static void *name_service(const char *name)
{
  network_card_t card = NULL;

  xbt_dict_get(network_card_set, name, (void *) &card);

  return card;
}

static const char *get_resource_name(void *resource_id)
{
  return ((network_card_t) resource_id)->name;
}

static int resource_used(void *resource_id)
{
  return lmm_constraint_used(maxmin_system,
			     ((network_link_t) resource_id)->constraint);
}

static void action_free(surf_action_t action)
{
  surf_action_network_t Action = (surf_action_network_t) action;

  xbt_swag_remove(action, action->state_set);
  lmm_variable_free(maxmin_system, Action->variable);
  xbt_free(action);

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

static void action_change_state(surf_action_t action,
				e_surf_action_state_t state)
{
  surf_action_change_state(action, state);
  return;
}

static xbt_heap_float_t share_resources(xbt_heap_float_t now)
{
  surf_action_network_t action = NULL;
  xbt_swag_t running_actions =
      surf_network_resource->common_public->states.running_action_set;
  xbt_maxmin_float_t min = -1;
  xbt_maxmin_float_t value = -1;
  lmm_solve(maxmin_system);

  action = xbt_swag_getFirst(running_actions);
  if (!action)
    return -1.0;
  value = lmm_variable_getvalue(action->variable);
  min = action->generic_action.remains / value;

  xbt_swag_foreach(action, running_actions) {
    value = action->latency + (action->generic_action.remains /
	lmm_variable_getvalue(action->variable));
    if (value < min)
      min = value;
  }

  return min;
}


static void update_actions_state(xbt_heap_float_t now,
				 xbt_heap_float_t delta)
{
  xbt_heap_float_t deltap = 0.0;
  surf_action_network_t action = NULL;
  surf_action_network_t next_action = NULL;
  xbt_swag_t running_actions =
      surf_network_resource->common_public->states.running_action_set;
  xbt_swag_t failed_actions =
      surf_network_resource->common_public->states.failed_action_set;

  xbt_swag_foreach_safe(action, next_action, running_actions) {
    deltap = delta;
    if(action->latency>0) {
      if(action->latency>deltap) {
	action->latency-=deltap;
	deltap = 0.0;
      } else {
	deltap -= action->latency;
	action->latency = 0.0;
      }
    }
    action->generic_action.remains -=
      lmm_variable_getvalue(action->variable) * deltap;

/*     if(action->generic_action.remains<.00001) action->generic_action.remains=0; */

    if (action->generic_action.remains <= 0) {
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else {			/* Need to check that none of the resource has failed */
      lmm_constraint_t cnst = NULL;
      int i = 0;
      network_link_t nw_link = NULL;

      while ((cnst =
	      lmm_get_cnst_from_var(maxmin_system, action->variable,
				    i++))) {
	nw_link = lmm_constraint_id(cnst);
	if (nw_link->state_current == SURF_NETWORK_LINK_OFF) {
	  action_change_state((surf_action_t) action, SURF_ACTION_FAILED);
	  break;
	}
      }
    }
  }

  xbt_swag_foreach_safe(action, next_action, failed_actions) {
    lmm_variable_disable(maxmin_system, action->variable);
  }

  return;
}

static void update_resource_state(void *id,
				  tmgr_trace_event_t event_type,
				  xbt_maxmin_float_t value)
{
  network_link_t nw_link = id;

/*   printf("[" XBT_HEAP_FLOAT_T "] Asking to update network card \"%s\" with value " */
/* 	 XBT_MAXMIN_FLOAT_T " for event %p\n", surf_get_clock(), nw_link->name, */
/* 	 value, event_type); */

  if (event_type == nw_link->bw_event) {
    nw_link->bw_current = value;
    lmm_update_constraint_bound(maxmin_system, nw_link->constraint,
				nw_link->bw_current);
  } else if (event_type == nw_link->lat_event) {
    nw_link->lat_current = value;
  } else if (event_type == nw_link->state_event) {
    if (value > 0)
      nw_link->state_current = SURF_NETWORK_LINK_ON;
    else
      nw_link->state_current = SURF_NETWORK_LINK_OFF;
  } else {
    CRITICAL0("Unknown event ! \n");
    xbt_abort();
  }

  return;
}

static surf_action_t communicate(void *src, void *dst,
				 xbt_maxmin_float_t size)
{
  surf_action_network_t action = NULL;
  network_card_t card_src = src;
  network_card_t card_dst = dst;
  int route_size = ROUTE_SIZE(card_src->id,card_dst->id);
  network_link_t *route = ROUTE(card_src->id,card_dst->id);
  int i;

  action = xbt_new0(s_surf_action_network_t, 1);

  action->generic_action.cost = size;
  action->generic_action.remains = size;
  action->generic_action.start = -1.0;
  action->generic_action.finish = -1.0;
  action->generic_action.callback = NULL;
  action->generic_action.resource_type =
      (surf_resource_t) surf_network_resource;

  action->generic_action.state_set =
    surf_network_resource->common_public->states.running_action_set;

  xbt_swag_insert(action, action->generic_action.state_set);

  action->variable = lmm_variable_new(maxmin_system, action, 1.0, -1.0, 
				      route_size);
  for(i=0; i<route_size; i++)
    lmm_expand(maxmin_system, route[i]->constraint, action->variable,
	       1.0);

  action->latency = 0.0;
  for(i=0; i<route_size; i++)
    action->latency += route[i]->lat_current;

  return (surf_action_t) action;
}

static void finalize(void)
{
  xbt_dict_free(&network_card_set);
  xbt_dict_free(&network_link_set);
  xbt_swag_free(surf_network_resource->common_public->states.ready_action_set);
  xbt_swag_free(surf_network_resource->common_public->states.
		running_action_set);
  xbt_swag_free(surf_network_resource->common_public->states.
		failed_action_set);
  xbt_swag_free(surf_network_resource->common_public->states.done_action_set);
  xbt_free(surf_network_resource->common_public);
  xbt_free(surf_network_resource->common_private);
  xbt_free(surf_network_resource->extension_public);

  xbt_free(surf_network_resource);
  surf_network_resource = NULL;
}

static void surf_network_resource_init_internal(void)
{
  s_surf_action_t action;

  surf_network_resource = xbt_new0(s_surf_network_resource_t, 1);

  surf_network_resource->common_private =
      xbt_new0(s_surf_resource_private_t, 1);
  surf_network_resource->common_public =
      xbt_new0(s_surf_resource_public_t, 1);
/*   surf_network_resource->extension_private = xbt_new0(s_surf_network_resource_extension_private_t,1); */
  surf_network_resource->extension_public =
      xbt_new0(s_surf_network_resource_extension_public_t, 1);

  surf_network_resource->common_public->states.ready_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_network_resource->common_public->states.running_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_network_resource->common_public->states.failed_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_network_resource->common_public->states.done_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_network_resource->common_public->name_service = name_service;
  surf_network_resource->common_public->get_resource_name =
      get_resource_name;
  surf_network_resource->common_public->action_get_state =
      surf_action_get_state;
  surf_network_resource->common_public->action_free = action_free;
  surf_network_resource->common_public->action_cancel = action_cancel;
  surf_network_resource->common_public->action_recycle = action_recycle;
  surf_network_resource->common_public->action_change_state =
      action_change_state;

  surf_network_resource->common_private->resource_used = resource_used;
  surf_network_resource->common_private->share_resources = share_resources;
  surf_network_resource->common_private->update_actions_state =
      update_actions_state;
  surf_network_resource->common_private->update_resource_state =
      update_resource_state;
  surf_network_resource->common_private->finalize = finalize;

  surf_network_resource->extension_public->communicate = communicate;

  network_link_set = xbt_dict_new();
  network_card_set = xbt_dict_new();

  xbt_assert0(maxmin_system, "surf_init has to be called first!");
}

void surf_network_resource_init(const char *filename)
{
  surf_network_resource_init_internal();
  parse_file(filename);
  xbt_dynar_push(resource_list, &surf_network_resource);
}
