#include "surf.hpp"
#include "workstation.hpp"
#include "network.hpp"
#include "instr/instr_private.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_kernel);

/*********
 * TOOLS *
 *********/
extern double NOW;

#ifdef CONTEXT_THREADS
static xbt_parmap_t surf_parmap = NULL; /* parallel map on models */
#endif

static double *surf_mins = NULL; /* return value of share_resources for each model */
static int surf_min_index;       /* current index in surf_mins */
static double surf_min;               /* duration determined by surf_solve */

void surf_presolve(void)
{
  double next_event_date = -1.0;
  tmgr_trace_event_t event = NULL;
  double value = -1.0;
  surf_resource_t resource = NULL;
  surf_model_t model = NULL;
  unsigned int iter;

  XBT_DEBUG
      ("First Run! Let's \"purge\" events and put models in the right state");
  while ((next_event_date = tmgr_history_next_date(history)) != -1.0) {
    if (next_event_date > NOW)
      break;
    while ((event =
            tmgr_history_get_next_event_leq(history, next_event_date,
                                            &value,
                                            (void **) &resource))) {
      if (value >= 0){
        resource->updateState(event, value, NOW);
      }
    }
  }
  xbt_dynar_foreach(model_list, iter, model)
      model->updateActionsState(NOW, 0.0);
}

static void surf_share_resources(surf_model_t model)
{
  double next_action_end = -1.0;
  int i = __sync_fetch_and_add(&surf_min_index, 1);
  if (strcmp(model->m_name.c_str(), "network NS3")) {
    XBT_DEBUG("Running for Resource [%s]", model->m_name.c_str());
    next_action_end = model->shareResources(NOW);
    XBT_DEBUG("Resource [%s] : next action end = %f",
        model->m_name.c_str(), next_action_end);
  }
  surf_mins[i] = next_action_end;
}

static void surf_update_actions_state(surf_model_t model)
{
  model->updateActionsState(NOW, surf_min);
}

double surf_solve(double max_date)
{
  surf_min = -1.0; /* duration */
  double next_event_date = -1.0;
  double model_next_action_end = -1.0;
  double value = -1.0;
  surf_resource_t resource = NULL;
  surf_model_t model = NULL;
  tmgr_trace_event_t event = NULL;
  unsigned int iter;

  if (max_date != -1.0 && max_date != NOW) {
    surf_min = max_date - NOW;
  }

  XBT_DEBUG("Looking for next action end for all models except NS3");

  if (surf_mins == NULL) {
    surf_mins = xbt_new(double, xbt_dynar_length(model_list));
  }
  surf_min_index = 0;

  /* sequential version */
  xbt_dynar_foreach(model_list, iter, model) {
    surf_share_resources(model);
  }

  unsigned i;
  for (i = 0; i < xbt_dynar_length(model_list); i++) {
    if ((surf_min < 0.0 || surf_mins[i] < surf_min)
        && surf_mins[i] >= 0.0) {
      surf_min = surf_mins[i];
    }
  }

  XBT_DEBUG("Min for resources (remember that NS3 don't update that value) : %f", surf_min);

  XBT_DEBUG("Looking for next trace event");

  do {
    XBT_DEBUG("Next TRACE event : %f", next_event_date);

    next_event_date = tmgr_history_next_date(history);

    if(!strcmp(surf_network_model->m_name.c_str(), "network NS3")){//FIXME: add surf_network_model->m_name &&
      if(next_event_date!=-1.0 && surf_min!=-1.0) {
        surf_min = MIN(next_event_date - NOW, surf_min);
      } else{
        surf_min = MAX(next_event_date - NOW, surf_min);
      }

      XBT_DEBUG("Run for network at most %f", surf_min);
      // run until min or next flow
      model_next_action_end = surf_network_model->shareResources(surf_min);

      XBT_DEBUG("Min for network : %f", model_next_action_end);
      if(model_next_action_end>=0.0)
        surf_min = model_next_action_end;
    }

    if (next_event_date < 0.0) {
      XBT_DEBUG("no next TRACE event. Stop searching for it");
      break;
    }

    if ((surf_min == -1.0) || (next_event_date > NOW + surf_min)) break;

    XBT_DEBUG("Updating models (min = %g, NOW = %g, next_event_date = %g)", surf_min, NOW, next_event_date);
    while ((event =
            tmgr_history_get_next_event_leq(history, next_event_date,
                                            &value,
                                            (void **) &resource))) {
      if (resource->isUsed()) {
        surf_min = next_event_date - NOW;
        XBT_DEBUG
            ("This event will modify model state. Next event set to %f",
             surf_min);
      }
      /* update state of model_obj according to new value. Does not touch lmm.
         It will be modified if needed when updating actions */
      XBT_DEBUG("Calling update_resource_state for resource %s with min %lf",
             resource->p_model->m_name.c_str(), surf_min);
      resource->updateState(event, value, next_event_date);
    }
  } while (1);

  /* FIXME: Moved this test to here to avoid stopping simulation if there are actions running on cpus and all cpus are with availability = 0.
   * This may cause an infinite loop if one cpu has a trace with periodicity = 0 and the other a trace with periodicity > 0.
   * The options are: all traces with same periodicity(0 or >0) or we need to change the way how the events are managed */
  if (surf_min == -1.0) {
  XBT_DEBUG("No next event at all. Bail out now.");
    return -1.0;
  }

  XBT_DEBUG("Duration set to %f", surf_min);

  NOW = NOW + surf_min;

  /* sequential version */
  xbt_dynar_foreach(model_list, iter, model) {
    surf_update_actions_state(model);
  }

#ifdef HAVE_TRACING
  TRACE_paje_dump_buffer (0);
#endif

  return surf_min;
}

XBT_INLINE double surf_get_clock(void)
{
  return NOW;
}

void routing_get_route_and_latency(sg_routing_edge_t src, sg_routing_edge_t dst,
                              xbt_dynar_t * route, double *latency){
  routing_platf->getRouteAndLatency(src, dst, route, latency);
}

/*********
 * MODEL *
 *********/

const char *surf_model_name(surf_model_t model){
  return model->m_name.c_str();
}

xbt_swag_t surf_model_done_action_set(surf_model_t model){
  return model->p_doneActionSet;
}

xbt_swag_t surf_model_failed_action_set(surf_model_t model){
  return model->p_failedActionSet;
}

xbt_swag_t surf_model_ready_action_set(surf_model_t model){
  return model->p_readyActionSet;
}

xbt_swag_t surf_model_running_action_set(surf_model_t model){
  return model->p_runningActionSet;
}

surf_action_t surf_workstation_model_execute_parallel_task(surf_workstation_model_t model,
		                                    int workstation_nb,
                                            void **workstation_list,
                                            double *computation_amount,
                                            double *communication_amount,
                                            double rate){
  return model->executeParallelTask(workstation_nb, workstation_list, computation_amount, communication_amount, rate);
}

surf_action_t surf_workstation_model_communicate(surf_workstation_model_t model, surf_workstation_CLM03_t src, surf_workstation_CLM03_t dst, double size, double rate){
  model->communicate(src, dst, size, rate);
}

xbt_dynar_t surf_workstation_model_get_route(surf_workstation_model_t model,
		                                     surf_workstation_t src, surf_workstation_t dst){
  return model->getRoute((WorkstationCLM03Ptr)surf_workstation_resource_priv(src),(WorkstationCLM03Ptr)surf_workstation_resource_priv(dst));
}

surf_action_t surf_network_model_communicate(surf_network_model_t model, sg_routing_edge_t src, sg_routing_edge_t dst, double size, double rate){
  model->communicate(src, dst, size, rate);
}

const char *surf_resource_name(surf_resource_t resource){
  return resource->m_name;
}

xbt_dict_t surf_resource_get_properties(surf_resource_t resource){
  return resource->m_properties;
}

e_surf_resource_state_t surf_resource_get_state(surf_resource_t resource){
  return resource->getState();
}

surf_action_t surf_workstation_sleep(surf_workstation_t resource, double duration){
  return ((surf_workstation_CLM03_t)surf_workstation_resource_priv(resource))->sleep(duration);
}

double surf_workstation_get_speed(surf_workstation_t resource, double load){
  return ((surf_workstation_CLM03_t)surf_workstation_resource_priv(resource))->getSpeed(load);
}

double surf_workstation_get_available_speed(surf_workstation_t resource){
  return ((surf_workstation_CLM03_t)surf_workstation_resource_priv(resource))->getAvailableSpeed();
}

int surf_workstation_get_core(surf_workstation_t resource){
  return ((surf_workstation_CLM03_t)surf_workstation_resource_priv(resource))->getCore();
}

surf_action_t surf_workstation_execute(surf_workstation_t resource, double size){
  return ((surf_workstation_CLM03_t)surf_workstation_resource_priv(resource))->execute(size);
}

surf_action_t surf_workstation_communicate(surf_workstation_t workstation_src, surf_workstation_t workstation_dst, double size, double rate){
  return surf_workstation_model->communicate((surf_workstation_CLM03_t)surf_workstation_resource_priv(workstation_src),(surf_workstation_CLM03_t)surf_workstation_resource_priv(workstation_dst), size, rate);
}

surf_action_t surf_workstation_open(surf_workstation_t workstation, const char* mount, const char* path){
  return ((surf_workstation_CLM03_t)surf_workstation_resource_priv(workstation))->open(mount, path);
}

surf_action_t surf_workstation_close(surf_workstation_t workstation, surf_file_t fd){
  return ((surf_workstation_CLM03_t)surf_workstation_resource_priv(workstation))->close(fd);
}

int surf_workstation_unlink(surf_workstation_t workstation, surf_file_t fd){
  return ((surf_workstation_CLM03_t)surf_workstation_resource_priv(workstation))->unlink(fd);
}

surf_action_t surf_workstation_ls(surf_workstation_t workstation, const char* mount, const char *path){
  return ((surf_workstation_CLM03_t)surf_workstation_resource_priv(workstation))->ls(mount, path);
}

size_t surf_workstation_get_size(surf_workstation_t workstation, surf_file_t fd){
  return ((surf_workstation_CLM03_t)surf_workstation_resource_priv(workstation))->getSize(fd);
}

surf_action_t surf_workstation_read(surf_workstation_t resource, void *ptr, size_t size, surf_file_t fd){
  return ((surf_workstation_CLM03_t)surf_workstation_resource_priv(resource))->read(ptr, size, fd);
}

surf_action_t surf_workstation_write(surf_workstation_t resource, const void *ptr, size_t size, surf_file_t fd){
  return ((surf_workstation_CLM03_t)surf_workstation_resource_priv(resource))->write(ptr, size, fd);
}

int surf_network_link_is_shared(surf_network_link_t link){
  return link->isShared();
}

double surf_network_link_get_bandwidth(surf_network_link_t link){
  return link->getBandwidth();
}

double surf_network_link_get_latency(surf_network_link_t link){
  return link->getLatency();
}

surf_action_t surf_cpu_execute(surf_cpu_t cpu, double size){
  return cpu->execute(size);
}

surf_action_t surf_cpu_sleep(surf_cpu_t cpu, double duration){
  return cpu->sleep(duration);
}

double surf_action_get_start_time(surf_action_t action){
  return action->m_start;
}

double surf_action_get_finish_time(surf_action_t action){
  return action->m_finish;
}

double surf_action_get_remains(surf_action_t action){
  return action->m_remains;
}

void surf_action_unref(surf_action_t action){
  action->unref();
}

void surf_action_suspend(surf_action_t action){
  action->suspend();
}

void surf_action_resume(surf_action_t action){
  action->suspend();
}

void surf_action_cancel(surf_action_t action){
  action->cancel();
}

void surf_action_set_priority(surf_action_t action, double priority){
  action->setPriority(priority);
}

void surf_action_set_category(surf_action_t action, const char *category){
  action->setCategory(category);
}

void *surf_action_get_data(surf_action_t action){
  return action->p_data;
}

void surf_action_set_data(surf_action_t action, void *data){
  action->p_data = data;
}

e_surf_action_state_t surf_action_get_state(surf_action_t action){
  return action->getState();
}

int surf_action_get_cost(surf_action_t action){
  return action->m_cost;
}

surf_file_t surf_storage_action_get_file(surf_storage_action_lmm_t action){
  return action->p_file;
}

xbt_dict_t surf_storage_action_get_ls_dict(surf_storage_action_lmm_t action){
  return action->p_lsDict;
}
