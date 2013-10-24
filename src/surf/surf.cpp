#include "surf_private.h"
#include "surf.hpp"
#include "network.hpp"
#include "cpu.hpp"
#include "workstation.hpp"
#include "simix/smx_host_private.h"
#include "surf_routing.hpp"
#include "simgrid/sg_config.h"
#include "mc/mc.h"

extern "C" {
XBT_LOG_NEW_CATEGORY(surf, "All SURF categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_kernel, surf,
                                "Logging specific to SURF (kernel)");
}

/*********
 * Utils *
 *********/

/* This function is a pimple that we ought to fix. But it won't be easy.
 *
 * The surf_solve() function does properly return the set of actions that changed.
 * Instead, each model change a global data, and then the caller of surf_solve must
 * pick into these sets of action_failed and action_done.
 *
 * This was not clean but ok as long as we didn't had to restart the processes when the resource comes back up.
 * We worked by putting sentinel actions on every resources we are interested in,
 * so that surf informs us if/when the corresponding resource fails.
 *
 * But this does not work to get Simix informed of when a resource comes back up, and this is where this pimple comes.
 * We have a set of resources that are currently down and for which simix needs to know when it comes back up.
 * And the current function is called *at every simulation step* to sweep over that set, searching for a resource
 * that was turned back up in the meanwhile. This is UGLY and slow.
 *
 * The proper solution would be to not rely on globals for the action_failed and action_done swags.
 * They must be passed as parameter by the caller (the handling of these actions in simix may let you
 * think that these two sets can be merged, but their handling in SimDag induce the contrary unless this
 * simdag code can check by itself whether the action is done of failed -- seems very doable, but yet more
 * cleanup to do).
 *
 * Once surf_solve() is passed the set of actions that changed, you want to add a new set of resources back up
 * as parameter to this function. You also want to add a boolean field "restart_watched" to each resource, and
 * make sure that whenever a resource with this field enabled comes back up, it's added to that set so that Simix
 * sees it and react accordingly. This would kill that need for surf to call simix.
 *
 */

static void remove_watched_host(void *key)
{
  xbt_dict_remove(watched_hosts_lib, *(char**)key);
}

void surf_watched_hosts(void)
{
  char *key;
  void *host;
  xbt_dict_cursor_t cursor;
  xbt_dynar_t hosts = xbt_dynar_new(sizeof(char*), NULL);

  XBT_DEBUG("Check for host SURF_RESOURCE_ON on watched_hosts_lib");
  xbt_dict_foreach(watched_hosts_lib, cursor, key, host)
  {
    if(SIMIX_host_get_state((smx_host_t)host) == SURF_RESOURCE_ON){
      XBT_INFO("Restart processes on host: %s", SIMIX_host_get_name((smx_host_t)host));
      SIMIX_host_autorestart((smx_host_t)host);
      xbt_dynar_push_as(hosts, char*, key);
    }
    else
      XBT_DEBUG("See SURF_RESOURCE_OFF on host: %s",key);
  }
  xbt_dynar_map(hosts, remove_watched_host);
  xbt_dynar_free(&hosts);
}


xbt_dynar_t model_list = NULL;
tmgr_history_t history = NULL;
lmm_system_t maxmin_system = NULL;
xbt_dynar_t surf_path = NULL;

/* Don't forget to update the option description in smx_config when you change this */
s_surf_model_description_t surf_network_model_description[] = {
  {"LV08",
   "Realistic network analytic model (slow-start modeled by multiplying latency by 10.4, bandwidth by .92; bottleneck sharing uses a payload of S=8775 for evaluating RTT). ",
   surf_network_model_init_LegrandVelho},
  {"Constant",
   "Simplistic network model where all communication take a constant time (one second). This model provides the lowest realism, but is (marginally) faster.",
   surf_network_model_init_Constant},
  {"SMPI",
   "Realistic network model specifically tailored for HPC settings (accurate modeling of slow start with correction factors on three intervals: < 1KiB, < 64 KiB, >= 64 KiB)",
   surf_network_model_init_SMPI},
  {"CM02",
   "Legacy network analytic model (Very similar to LV08, but without corrective factors. The timings of small messages are thus poorly modeled).",
   surf_network_model_init_CM02},
#ifdef HAVE_GTNETS
  {"GTNets",
   "Network pseudo-model using the GTNets simulator instead of an analytic model",
   surf_network_model_init_GTNETS},
#endif
#ifdef HAVE_NS3
  {"NS3",
   "Network pseudo-model using the NS3 tcp model instead of an analytic model",
  surf_network_model_init_NS3},
#endif
  {"Reno",
   "Model from Steven H. Low using lagrange_solve instead of lmm_solve (experts only; check the code for more info).",
   surf_network_model_init_Reno},
  {"Reno2",
   "Model from Steven H. Low using lagrange_solve instead of lmm_solve (experts only; check the code for more info).",
   surf_network_model_init_Reno2},
  {"Vegas",
   "Model from Steven H. Low using lagrange_solve instead of lmm_solve (experts only; check the code for more info).",
   surf_network_model_init_Vegas},
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_surf_model_description_t surf_cpu_model_description[] = {
  {"Cas01",
   "Simplistic CPU model (time=size/power).",
   surf_cpu_model_init_Cas01},
  {NULL, NULL,  NULL}      /* this array must be NULL terminated */
};

s_surf_model_description_t surf_workstation_model_description[] = {
  {"default",
   "Default workstation model. Currently, CPU:Cas01 and network:LV08 (with cross traffic enabled)",
   surf_workstation_model_init_current_default},
  {"compound",
   "Workstation model that is automatically chosen if you change the network and CPU models",
   surf_workstation_model_init_compound},
  {"ptask_L07", "Workstation model somehow similar to Cas01+CM02 but allowing parallel tasks",
   surf_workstation_model_init_ptask_L07},
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_surf_model_description_t surf_optimization_mode_description[] = {
  {"Lazy",
   "Lazy action management (partial invalidation in lmm + heap in action remaining).",
   NULL},
  {"TI",
   "Trace integration. Highly optimized mode when using availability traces (only available for the Cas01 CPU model for now).",
    NULL},
  {"Full",
   "Full update of remaining and variables. Slow but may be useful when debugging.",
   NULL},
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_surf_model_description_t surf_storage_model_description[] = {
  {"default",
   "Simplistic storage model.",
   surf_storage_model_init_default},
  {NULL, NULL,  NULL}      /* this array must be NULL terminated */
};

#ifdef CONTEXT_THREADS
static xbt_parmap_t surf_parmap = NULL; /* parallel map on models */
#endif

static double *surf_mins = NULL; /* return value of share_resources for each model */
static int surf_min_index;       /* current index in surf_mins */
static double min;               /* duration determined by surf_solve */

double NOW = 0;

double surf_get_clock(void)
{
  return NOW;
}

/*TODO: keepit void surf_watched_hosts(void)
{
  char *key;
  void *_host;
  smx_host_t host;
  xbt_dict_cursor_t cursor;
  xbt_dynar_t hosts = xbt_dynar_new(sizeof(char*), NULL);

  XBT_DEBUG("Check for host SURF_RESOURCE_ON on watched_hosts_lib");
  xbt_dict_foreach(watched_hosts_lib,cursor,key,_host)
  {
    host = (smx_host_t) host;
    if(SIMIX_host_get_state(host) == SURF_RESOURCE_ON){
      XBT_INFO("Restart processes on host: %s",SIMIX_host_get_name(host));
      SIMIX_host_autorestart(host);
      xbt_dynar_push_as(hosts, char*, key);
    }
    else
      XBT_DEBUG("See SURF_RESOURCE_OFF on host: %s",key);
  }
  xbt_dynar_map(hosts, remove_watched_host);
  xbt_dynar_free(&hosts);
}*/

#ifdef _XBT_WIN32
# define FILE_DELIM "\\"
#else
# define FILE_DELIM "/"         /* FIXME: move to better location */
#endif

FILE *surf_fopen(const char *name, const char *mode)
{
  unsigned int cpt;
  char *path_elm = NULL;
  char *buff;
  FILE *file = NULL;

  xbt_assert(name);

  if (__surf_is_absolute_file_path(name))       /* don't mess with absolute file names */
    return fopen(name, mode);

  /* search relative files in the path */
  xbt_dynar_foreach(surf_path, cpt, path_elm) {
    buff = bprintf("%s" FILE_DELIM "%s", path_elm, name);
    file = fopen(buff, mode);
    free(buff);

    if (file)
      return file;
  }
  return NULL;
}

/*
 * Returns the initial path. On Windows the initial path is
 * the current directory for the current process in the other
 * case the function returns "./" that represents the current
 * directory on Unix/Linux platforms.
 */

const char *__surf_get_initial_path(void)
{

#ifdef _XBT_WIN32
  unsigned i;
  char current_directory[MAX_PATH + 1] = { 0 };
  unsigned int len = GetCurrentDirectory(MAX_PATH + 1, current_directory);
  char root[4] = { 0 };

  if (!len)
    return NULL;

  strncpy(root, current_directory, 3);

  for (i = 0; i < MAX_DRIVE; i++) {
    if (toupper(root[0]) == disk_drives_letter_table[i][0])
      return disk_drives_letter_table[i];
  }

  return NULL;
#else
  return "./";
#endif
}

/* The __surf_is_absolute_file_path() returns 1 if
 * file_path is a absolute file path, in the other
 * case the function returns 0.
 */
int __surf_is_absolute_file_path(const char *file_path)
{
#ifdef _XBT_WIN32
  WIN32_FIND_DATA wfd = { 0 };
  HANDLE hFile = FindFirstFile(file_path, &wfd);

  if (INVALID_HANDLE_VALUE == hFile)
    return 0;

  FindClose(hFile);
  return 1;
#else
  return (file_path[0] == '/');
#endif
}

/** Displays the long description of all registered models, and quit */
void model_help(const char *category, s_surf_model_description_t * table)
{
  int i;
  printf("Long description of the %s models accepted by this simulator:\n",
         category);
  for (i = 0; table[i].name; i++)
    printf("  %s: %s\n", table[i].name, table[i].description);
}

int find_model_description(s_surf_model_description_t * table,
                           const char *name)
{
  int i;
  char *name_list = NULL;

  for (i = 0; table[i].name; i++)
    if (!strcmp(name, table[i].name)) {
      return i;
    }
  name_list = strdup(table[0].name);
  for (i = 1; table[i].name; i++) {
    name_list = (char *) xbt_realloc(name_list, strlen(name_list) + strlen(table[i].name) + 3);
    strcat(name_list, ", ");
    strcat(name_list, table[i].name);
  }
  xbt_die("Model '%s' is invalid! Valid models are: %s.", name, name_list);
  return -1;
}

static XBT_INLINE void routing_asr_host_free(void *p)
{
  delete ((RoutingEdgePtr) p);
}

static XBT_INLINE void routing_asr_prop_free(void *p)
{
  xbt_dict_t elm = (xbt_dict_t) p;
  xbt_dict_free(&elm);
}

static XBT_INLINE void surf_cpu_free(void *r)
{
  delete dynamic_cast<CpuPtr>(static_cast<ResourcePtr>(r));
}

static XBT_INLINE void surf_link_free(void *r)
{
  delete dynamic_cast<NetworkCm02LinkPtr>(static_cast<ResourcePtr>(r));
}

static XBT_INLINE void surf_workstation_free(void *r)
{
  delete dynamic_cast<WorkstationCLM03Ptr>(static_cast<ResourcePtr>(r));
}


void sg_version(int *ver_major,int *ver_minor,int *ver_patch) {
  *ver_major = SIMGRID_VERSION_MAJOR;
  *ver_minor = SIMGRID_VERSION_MINOR;
  *ver_patch = SIMGRID_VERSION_PATCH;
}

void surf_init(int *argc, char **argv)
{
  XBT_DEBUG("Create all Libs");
  host_lib = xbt_lib_new();
  link_lib = xbt_lib_new();
  as_router_lib = xbt_lib_new();
  storage_lib = xbt_lib_new();
  storage_type_lib = xbt_lib_new();
  watched_hosts_lib = xbt_dict_new();

  XBT_DEBUG("Add routing levels");
  ROUTING_HOST_LEVEL = xbt_lib_add_level(host_lib,routing_asr_host_free);
  ROUTING_ASR_LEVEL  = xbt_lib_add_level(as_router_lib,routing_asr_host_free);
  ROUTING_PROP_ASR_LEVEL = xbt_lib_add_level(as_router_lib,routing_asr_prop_free);

  XBT_DEBUG("Add SURF levels");
  SURF_CPU_LEVEL = xbt_lib_add_level(host_lib,surf_cpu_free);
  SURF_WKS_LEVEL = xbt_lib_add_level(host_lib,surf_workstation_free);
  SURF_LINK_LEVEL = xbt_lib_add_level(link_lib,surf_link_free);

  xbt_init(argc, argv);
  if (!model_list)
    model_list = xbt_dynar_new(sizeof(surf_model_private_t), NULL);
  if (!history)
    history = tmgr_history_new();

#ifdef HAVE_TRACING
  TRACE_add_start_function(TRACE_surf_alloc);
  TRACE_add_end_function(TRACE_surf_release);
#endif

  sg_config_init(argc, argv);

  surf_action_init();
  if (MC_is_active())
    MC_memory_init();
}

void surf_exit(void)
{
  unsigned int iter;
  ModelPtr model = NULL;

  sg_config_finalize();

  xbt_dynar_foreach(model_list, iter, model)
    delete model;
  xbt_dynar_free(&model_list);
  routing_exit();

  if (maxmin_system) {
    lmm_system_free(maxmin_system);
    maxmin_system = NULL;
  }
  if (history) {
    tmgr_history_free(history);
    history = NULL;
  }
  surf_action_exit();

#ifdef CONTEXT_THREADS
  xbt_parmap_destroy(surf_parmap);
  xbt_free(surf_mins);
  surf_mins = NULL;
#endif

  xbt_dynar_free(&surf_path);

  xbt_lib_free(&host_lib);
  xbt_lib_free(&link_lib);
  xbt_lib_free(&as_router_lib);
  xbt_lib_free(&storage_lib);
  xbt_lib_free(&storage_type_lib);

  xbt_dict_free(&watched_hosts_lib);

  tmgr_finalize();
  surf_parse_lex_destroy();
  surf_parse_free_callbacks();

  NOW = 0;                      /* Just in case the user plans to restart the simulation afterward */
}
/*********
 * Model *
 *********/

Model::Model(string name)
 : m_name(name), m_resOnCB(0), m_resOffCB(0),
   m_actSuspendCB(0), m_actCancelCB(0), m_actResumeCB(0),
   p_maxminSystem(0)
{
  ActionPtr action;
  p_readyActionSet = xbt_swag_new(xbt_swag_offset(*action, p_stateHookup));
  p_runningActionSet = xbt_swag_new(xbt_swag_offset(*action, p_stateHookup));
  p_failedActionSet = xbt_swag_new(xbt_swag_offset(*action, p_stateHookup));
  p_doneActionSet = xbt_swag_new(xbt_swag_offset(*action, p_stateHookup));

  p_modifiedSet = NULL;
  p_actionHeap = NULL;
  p_updateMechanism = UM_UNDEFINED;
  m_selectiveUpdate = 0;
}

Model::~Model(){
xbt_swag_free(p_readyActionSet);
xbt_swag_free(p_runningActionSet);
xbt_swag_free(p_failedActionSet);
xbt_swag_free(p_doneActionSet);
}

double Model::shareResources(double now)
{
  //FIXME: set the good function once and for all
  if (p_updateMechanism == UM_LAZY)
	return shareResourcesLazy(now);
  else if (p_updateMechanism == UM_FULL)
	return shareResourcesFull(now);
  else
	xbt_die("Invalid cpu update mechanism!");
}

double Model::shareResourcesLazy(double now)
{
  ActionLmmPtr action = NULL;
  double min = -1;
  double value;

  XBT_DEBUG
      ("Before share resources, the size of modified actions set is %d",
       xbt_swag_size(p_modifiedSet));

  lmm_solve(p_maxminSystem);

  XBT_DEBUG
      ("After share resources, The size of modified actions set is %d",
       xbt_swag_size(p_modifiedSet));

  while((action = static_cast<ActionLmmPtr>(xbt_swag_extract(p_modifiedSet)))) {
    int max_dur_flag = 0;

    if (action->p_stateSet != p_runningActionSet)
      continue;

    /* bogus priority, skip it */
    if (action->m_priority <= 0)
      continue;

    action->updateRemainingLazy(now);

    min = -1;
    value = lmm_variable_getvalue(action->p_variable);
    if (value > 0) {
      if (action->m_remains > 0) {
        value = action->m_remains / value;
        min = now + value;
      } else {
        value = 0.0;
        min = now;
      }
    }

    if ((action->m_maxDuration != NO_MAX_DURATION)
        && (min == -1
            || action->m_start +
            action->m_maxDuration < min)) {
      min = action->m_start +
          action->m_maxDuration;
      max_dur_flag = 1;
    }

    XBT_DEBUG("Action(%p) Start %lf Finish %lf Max_duration %lf", action,
        action->m_start, now + value,
        action->m_maxDuration);

    if (min != -1) {
      action->heapRemove(p_actionHeap);
      action->heapInsert(p_actionHeap, min, max_dur_flag ? MAX_DURATION : NORMAL);
      XBT_DEBUG("Insert at heap action(%p) min %lf now %lf", action, min,
                now);
    } else DIE_IMPOSSIBLE;
  }

  //hereafter must have already the min value for this resource model
  if (xbt_heap_size(p_actionHeap) > 0)
    min = xbt_heap_maxkey(p_actionHeap) - now;
  else
    min = -1;

  XBT_DEBUG("The minimum with the HEAP %lf", min);

  return min;
}

double Model::shareResourcesFull(double now) {
  THROW_UNIMPLEMENTED;
}


double Model::shareResourcesMaxMin(xbt_swag_t running_actions,
                          lmm_system_t sys,
                          void (*solve) (lmm_system_t))
{
  void *_action = NULL;
  ActionLmmPtr action = NULL;
  double min = -1;
  double value = -1;

  solve(sys);

  xbt_swag_foreach(_action, running_actions) {
    action = dynamic_cast<ActionLmmPtr>(static_cast<ActionPtr>(_action));
    value = lmm_variable_getvalue(action->p_variable);
    if ((value > 0) || (action->m_maxDuration >= 0))
      break;
  }

  if (!_action)
    return -1.0;

  if (value > 0) {
    if (action->m_remains > 0)
      min = action->m_remains / value;
    else
      min = 0.0;
    if ((action->m_maxDuration >= 0) && (action->m_maxDuration < min))
      min = action->m_maxDuration;
  } else
    min = action->m_maxDuration;


  for (_action = xbt_swag_getNext(static_cast<ActionPtr>(action), running_actions->offset);
       _action;
       _action = xbt_swag_getNext(static_cast<ActionPtr>(action), running_actions->offset)) {
	action = dynamic_cast<ActionLmmPtr>(static_cast<ActionPtr>(_action));
    value = lmm_variable_getvalue(action->p_variable);
    if (value > 0) {
      if (action->m_remains > 0)
        value = action->m_remains / value;
      else
        value = 0.0;
      if (value < min) {
        min = value;
        XBT_DEBUG("Updating min (value) with %p: %f", action, min);
      }
    }
    if ((action->m_maxDuration >= 0) && (action->m_maxDuration < min)) {
      min = action->m_maxDuration;
      XBT_DEBUG("Updating min (duration) with %p: %f", action, min);
    }
  }
  XBT_DEBUG("min value : %f", min);

  return min;
}

void Model::updateActionsState(double now, double delta)
{
  if (p_updateMechanism == UM_FULL)
	updateActionsStateFull(now, delta);
  else if (p_updateMechanism == UM_LAZY)
	updateActionsStateLazy(now, delta);
  else
	xbt_die("Invalid cpu update mechanism!");
}

void Model::updateActionsStateLazy(double now, double delta)
{

}

void Model::updateActionsStateFull(double now, double delta)
{

}


void Model::addTurnedOnCallback(ResourceCallback rc)
{
  m_resOnCB = rc;
}

void Model::notifyResourceTurnedOn(ResourcePtr r)
{
  m_resOnCB(r);
}

void Model::addTurnedOffCallback(ResourceCallback rc)
{
  m_resOffCB = rc;
}

void Model::notifyResourceTurnedOff(ResourcePtr r)
{
  m_resOffCB(r);
}

void Model::addActionCancelCallback(ActionCallback ac)
{
  m_actCancelCB = ac;
}

void Model::notifyActionCancel(ActionPtr a)
{
  m_actCancelCB(a);
}

void Model::addActionResumeCallback(ActionCallback ac)
{
  m_actResumeCB = ac;
}

void Model::notifyActionResume(ActionPtr a)
{
  m_actResumeCB(a);
}

void Model::addActionSuspendCallback(ActionCallback ac)
{
  m_actSuspendCB = ac;
}

void Model::notifyActionSuspend(ActionPtr a)
{
  m_actSuspendCB(a);
}


/************
 * Resource *
 ************/

Resource::Resource(surf_model_t model, const char *name, xbt_dict_t props)
  : m_name(xbt_strdup(name)), m_running(true), p_model(model), m_properties(props)
{}

Resource::Resource(){
  //FIXME:free(m_name);
  //FIXME:xbt_dict_free(&m_properties);
}

const char *Resource::getName()
{
  return m_name;
}

xbt_dict_t Resource::getProperties()
{
  return m_properties;
}

e_surf_resource_state_t Resource::getState()
{
  return p_stateCurrent;
}

bool Resource::isOn()
{
  return m_running;
}

void Resource::turnOn()
{
  if (!m_running) {
    m_running = true;
    p_model->notifyResourceTurnedOn(this);
  }
}

void Resource::turnOff()
{
  if (m_running) {
    m_running = false;
    p_model->notifyResourceTurnedOff(this);
  }
}

ResourceLmm::ResourceLmm(surf_model_t model, const char *name, xbt_dict_t props,
                         lmm_system_t system,
                         double constraint_value,
                         tmgr_history_t history,
                         e_surf_resource_state_t state_init,
                         tmgr_trace_t state_trace,
                         double metric_peak,
                         tmgr_trace_t metric_trace)
  : Resource(model, name, props)
{
  p_constraint = lmm_constraint_new(system, this, constraint_value);
  p_stateCurrent = state_init;
  if (state_trace)
    p_stateEvent = tmgr_history_add_trace(history, state_trace, 0.0, 0, static_cast<ResourcePtr>(this));
  p_power.scale = 1.0;
  p_power.peak = metric_peak;
  if (metric_trace)
    p_power.event = tmgr_history_add_trace(history, metric_trace, 0.0, 0, static_cast<ResourcePtr>(this));
}

/**********
 * Action *
 **********/

const char *surf_action_state_names[6] = {
  "SURF_ACTION_READY",
  "SURF_ACTION_RUNNING",
  "SURF_ACTION_FAILED",
  "SURF_ACTION_DONE",
  "SURF_ACTION_TO_FREE",
  "SURF_ACTION_NOT_IN_THE_SYSTEM"
};

/**
 * \brief Initializes the action module of Surf.
 */
void surf_action_init(void) {

  /* the action mallocator will always provide actions of the following size,
   * so this size should be set to the maximum size of the surf action structures
   */
  /*FIXME:action_mallocator_allocated_size = sizeof(s_surf_action_network_CM02_t);
  action_mallocator = xbt_mallocator_new(65536, surf_action_mallocator_new_f,
      surf_action_mallocator_free_f, surf_action_mallocator_reset_f);*/
}

/**
 * \brief Uninitializes the action module of Surf.
 */
void surf_action_exit(void) {
  //FIXME:xbt_mallocator_free(action_mallocator);
}

Action::Action(){}

Action::Action(ModelPtr model, double cost, bool failed):
	 m_cost(cost), p_model(model), m_failed(failed), m_remains(cost),
	 m_refcount(1), m_priority(1.0), m_maxDuration(NO_MAX_DURATION),
	 m_start(surf_get_clock()), m_finish(-1.0)
{
  #ifdef HAVE_TRACING
    p_category = NULL;
  #endif
  p_stateHookup.prev = 0;
  p_stateHookup.next = 0;
  if (failed)
    p_stateSet = p_model->p_failedActionSet;
  else
    p_stateSet = p_model->p_runningActionSet;

  xbt_swag_insert(this, p_stateSet);
}

Action::~Action() {}

int Action::unref(){
  DIE_IMPOSSIBLE;
}

void Action::cancel(){
  DIE_IMPOSSIBLE;
}

void Action::recycle(){
  DIE_IMPOSSIBLE;
}

e_surf_action_state_t Action::getState()
{
  if (p_stateSet ==  p_model->p_readyActionSet)
    return SURF_ACTION_READY;
  if (p_stateSet ==  p_model->p_runningActionSet)
    return SURF_ACTION_RUNNING;
  if (p_stateSet ==  p_model->p_failedActionSet)
    return SURF_ACTION_FAILED;
  if (p_stateSet ==  p_model->p_doneActionSet)
    return SURF_ACTION_DONE;
  return SURF_ACTION_NOT_IN_THE_SYSTEM;
}

void Action::setState(e_surf_action_state_t state)
{
  //surf_action_state_t action_state = &(action->model_type->states);
  XBT_IN("(%p,%s)", this, surf_action_state_names[state]);
  xbt_swag_remove(this, p_stateSet);

  if (state == SURF_ACTION_READY)
    p_stateSet = p_model->p_readyActionSet;
  else if (state == SURF_ACTION_RUNNING)
    p_stateSet = p_model->p_runningActionSet;
  else if (state == SURF_ACTION_FAILED)
    p_stateSet = p_model->p_failedActionSet;
  else if (state == SURF_ACTION_DONE)
    p_stateSet = p_model->p_doneActionSet;
  else
    p_stateSet = NULL;

  if (p_stateSet)
    xbt_swag_insert(this, p_stateSet);
  XBT_OUT();
}

double Action::getStartTime()
{
  return m_start;
}

double Action::getFinishTime()
{
  /* keep the function behavior, some models (cpu_ti) change the finish time before the action end */
  return m_remains == 0 ? m_finish : -1;
}

double Action::getRemains()
{
  XBT_IN("(%p)", this);
  XBT_OUT();
  return m_remains;
}

void Action::setData(void* data)
{
  p_data = data;
}

#ifdef HAVE_TRACING
void Action::setCategory(const char *category)
{
  XBT_IN("(%p,%s)", this, category);
  p_category = xbt_strdup(category);
  XBT_OUT();
}
#endif

void Action::ref(){
  m_refcount++;
}

void ActionLmm::setMaxDuration(double duration)
{
  XBT_IN("(%p,%g)", this, duration);
  m_maxDuration = duration;
  if (p_model->p_updateMechanism == UM_LAZY)      // remove action from the heap
    heapRemove(p_model->p_actionHeap);
  XBT_OUT();
}

void ActionLmm::gapRemove() {}

void ActionLmm::setPriority(double priority)
{
  XBT_IN("(%p,%g)", this, priority);
  m_priority = priority;
  lmm_update_variable_weight(p_model->p_maxminSystem, p_variable, priority);

  if (p_model->p_updateMechanism == UM_LAZY)
	heapRemove(p_model->p_actionHeap);
  XBT_OUT();
}

void ActionLmm::cancel(){
  setState(SURF_ACTION_FAILED);
  if (p_model->p_updateMechanism == UM_LAZY) {
    xbt_swag_remove(this, p_model->p_modifiedSet);
    heapRemove(p_model->p_actionHeap);
  }
}

int ActionLmm::unref(){
  m_refcount--;
  if (!m_refcount) {
	xbt_swag_remove(static_cast<ActionPtr>(this), p_stateSet);
	if (p_variable)
	  lmm_variable_free(p_model->p_maxminSystem, p_variable);
	if (p_model->p_updateMechanism == UM_LAZY) {
	  /* remove from heap */
	  heapRemove(p_model->p_actionHeap);
	  xbt_swag_remove(this, p_model->p_modifiedSet);
    }
#ifdef HAVE_TRACING
    xbt_free(p_category);
#endif
	delete this;
	return 1;
  }
  return 0;
}

void ActionLmm::suspend()
{
  XBT_IN("(%p)", this);
  if (m_suspended != 2) {
    lmm_update_variable_weight(p_model->p_maxminSystem, p_variable, 0.0);
    m_suspended = 1;
    if (p_model->p_updateMechanism == UM_LAZY)
      heapRemove(p_model->p_actionHeap);
  }
  XBT_OUT();
}

void ActionLmm::resume()
{
  XBT_IN("(%p)", this);
  if (m_suspended != 2) {
    lmm_update_variable_weight(p_model->p_maxminSystem, p_variable, m_priority);
    m_suspended = 0;
    if (p_model->p_updateMechanism == UM_LAZY)
      heapRemove(p_model->p_actionHeap);
  }
  XBT_OUT();
}

bool ActionLmm::isSuspended()
{
  return m_suspended == 1;
}
/* insert action on heap using a given key and a hat (heap_action_type)
 * a hat can be of three types for communications:
 *
 * NORMAL = this is a normal heap entry stating the date to finish transmitting
 * LATENCY = this is a heap entry to warn us when the latency is payed
 * MAX_DURATION =this is a heap entry to warn us when the max_duration limit is reached
 */
void ActionLmm::heapInsert(xbt_heap_t heap, double key, enum heap_action_type hat)
{
  m_hat = hat;
  xbt_heap_push(heap, this, key);
}

void ActionLmm::heapRemove(xbt_heap_t heap)
{
  m_hat = NOTSET;
  if (m_indexHeap >= 0) {
    xbt_heap_remove(heap, m_indexHeap);
  }
}

/* added to manage the communication action's heap */
void surf_action_lmm_update_index_heap(void *action, int i) {
  ((ActionLmmPtr)action)->updateIndexHeap(i);
}

void ActionLmm::updateIndexHeap(int i) {
  m_indexHeap = i;
}

double ActionLmm::getRemains()
{
  XBT_IN("(%p)", this);
  /* update remains before return it */
  if (p_model->p_updateMechanism == UM_LAZY)      /* update remains before return it */
    updateRemainingLazy(surf_get_clock());
  XBT_OUT();
  return m_remains;
}

//FIXME split code in the right places
void ActionLmm::updateRemainingLazy(double now)
{
  double delta = 0.0;

  if(p_model == static_cast<ModelPtr>(surf_network_model))
  {
    if (m_suspended != 0)
      return;
  }
  else
  {
    xbt_assert(p_stateSet == p_model->p_runningActionSet,
        "You're updating an action that is not running.");

      /* bogus priority, skip it */
    xbt_assert(m_priority > 0,
        "You're updating an action that seems suspended.");
  }

  delta = now - m_lastUpdate;

  if (m_remains > 0) {
    XBT_DEBUG("Updating action(%p): remains was %lf, last_update was: %lf", this, m_remains, m_lastUpdate);
    double_update(&m_remains, m_lastValue * delta);

#ifdef HAVE_TRACING
    if (p_model == static_cast<ModelPtr>(surf_cpu_model) && TRACE_is_enabled()) {
      ResourcePtr cpu = static_cast<ResourcePtr>(lmm_constraint_id(lmm_get_cnst_from_var(p_model->p_maxminSystem, p_variable, 0)));
      TRACE_surf_host_set_utilization(cpu->m_name, p_category, m_lastValue, m_lastUpdate, now - m_lastUpdate);
    }
#endif
    XBT_DEBUG("Updating action(%p): remains is now %lf", this, m_remains);
  }

  if(p_model == static_cast<ModelPtr>(surf_network_model))
  {
    if (m_maxDuration != NO_MAX_DURATION)
      double_update(&m_maxDuration, delta);

    //FIXME: duplicated code
    if ((m_remains <= 0) &&
        (lmm_get_variable_weight(p_variable) > 0)) {
      m_finish = surf_get_clock();
      setState(SURF_ACTION_DONE);
      heapRemove(p_model->p_actionHeap);
    } else if (((m_maxDuration != NO_MAX_DURATION)
        && (m_maxDuration <= 0))) {
      m_finish = surf_get_clock();
      setState(SURF_ACTION_DONE);
      heapRemove(p_model->p_actionHeap);
    }
  }

  m_lastUpdate = now;
  m_lastValue = lmm_variable_getvalue(p_variable);
}

/*void Action::cancel()
{
  p_model->notifyActionCancel(this);
}

void Action::suspend()
{
  p_model->notifyActionSuspend(this);
}

void Action::resume()
{
  p_model->notifyActionResume(this);
}

bool Action::isSuspended()
{
  return false;
}*/

