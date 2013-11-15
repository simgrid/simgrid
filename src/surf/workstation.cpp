#include "workstation.hpp"
#include "vm_workstation.hpp"
#include "cpu_cas01.hpp"
#include "simgrid/sg_config.h"

extern "C" {
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_workstation, surf,
                                "Logging specific to the SURF workstation module");
}

WorkstationModelPtr surf_workstation_model = NULL;

//FIXME:Faire hériter ou composer de cup et network

/*************
 * CallBacks *
 *************/

static void workstation_new(sg_platf_host_cbarg_t host){
  surf_workstation_model->createResource(host->id);
}

/*********
 * Model *
 *********/

void surf_workstation_model_init_current_default(void)
{
  surf_workstation_model = new WorkstationModel();
  xbt_cfg_setdefault_boolean(_sg_cfg_set, "network/crosstraffic", "yes");
  surf_cpu_model_init_Cas01();
  surf_network_model_init_LegrandVelho();

  ModelPtr model = static_cast<ModelPtr>(surf_workstation_model);
  xbt_dynar_push(model_list, &model);
  xbt_dynar_push(model_list_invoke, &model);
  sg_platf_host_add_cb(workstation_new);
}

void surf_workstation_model_init_compound()
{

  xbt_assert(surf_cpu_model_pm, "No CPU model defined yet!");
  xbt_assert(surf_network_model, "No network model defined yet!");
  surf_workstation_model = new WorkstationModel();

  ModelPtr model = static_cast<ModelPtr>(surf_workstation_model);
  xbt_dynar_push(model_list, &model);
  xbt_dynar_push(model_list_invoke, &model);
  sg_platf_host_add_cb(workstation_new);
}

WorkstationModel::WorkstationModel() : Model("Workstation") {
}

WorkstationModel::~WorkstationModel() {
}

void WorkstationModel::parseInit(sg_platf_host_cbarg_t host){
  createResource(host->id);
}

WorkstationCLM03Ptr WorkstationModel::createResource(string name){

  WorkstationCLM03Ptr workstation = new WorkstationCLM03(surf_workstation_model, name.c_str(), NULL,
		  (xbt_dynar_t)xbt_lib_get_or_null(storage_lib, name.c_str(), ROUTING_STORAGE_HOST_LEVEL),
		  (RoutingEdgePtr)xbt_lib_get_or_null(host_lib, name.c_str(), ROUTING_HOST_LEVEL),
		  dynamic_cast<CpuPtr>(static_cast<ResourcePtr>(xbt_lib_get_or_null(host_lib, name.c_str(), SURF_CPU_LEVEL))));
  XBT_DEBUG("Create workstation %s with %ld mounted disks", name.c_str(), xbt_dynar_length(workstation->p_storage));
  xbt_lib_set(host_lib, name.c_str(), SURF_WKS_LEVEL, static_cast<ResourcePtr>(workstation));
  return workstation;
}

/* Each VM has a dummy CPU action on the PM layer. This CPU action works as the
 * constraint (capacity) of the VM in the PM layer. If the VM does not have any
 * active task, the dummy CPU action must be deactivated, so that the VM does
 * not get any CPU share in the PM layer. */
void WorkstationModel::adjustWeightOfDummyCpuActions()
{
  /* iterate for all hosts including virtual machines */
  xbt_lib_cursor_t cursor;
  char *key;
  void **ind_host;

  xbt_lib_foreach(host_lib, cursor, key, ind_host) {
    WorkstationCLM03Ptr ws_clm03 = dynamic_cast<WorkstationCLM03Ptr>(
    		                       static_cast<ResourcePtr>(ind_host[SURF_WKS_LEVEL]));
    CpuCas01LmmPtr cpu_cas01 = dynamic_cast<CpuCas01LmmPtr>(
                               static_cast<ResourcePtr>(ind_host[SURF_CPU_LEVEL]));

    if (!ws_clm03)
      continue;
    /* skip if it is not a virtual machine */
    if (ws_clm03->p_model != static_cast<ModelPtr>(surf_vm_workstation_model))
      continue;
    xbt_assert(cpu_cas01, "cpu-less workstation");

    /* It is a virtual machine, so we can cast it to workstation_VM2013_t */
    WorkstationVM2013Ptr ws_vm2013 = dynamic_cast<WorkstationVM2013Ptr>(ws_clm03);

    int is_active = lmm_constraint_used(cpu_cas01->p_model->p_maxminSystem, cpu_cas01->p_constraint);
    // int is_active_old = constraint_is_active(cpu_cas01);

    // {
    //   xbt_assert(is_active == is_active_old, "%d %d", is_active, is_active_old);
    // }

    if (is_active) {
      /* some tasks exist on this VM */
      XBT_DEBUG("set the weight of the dummy CPU action on PM to 1");

      /* FIXME: we shoud use lmm_update_variable_weight() ? */
      /* FIXME: If we assgign 1.05 and 0.05, the system makes apparently wrong values. */
      surf_action_set_priority(ws_vm2013->p_action, 1);

    } else {
      /* no task exits on this VM */
      XBT_DEBUG("set the weight of the dummy CPU action on PM to 0");

      surf_action_set_priority(ws_vm2013->p_action, 0);
    }
  }
}

double WorkstationModel::shareResources(double now){
  adjustWeightOfDummyCpuActions();

  double min_by_cpu = surf_cpu_model_pm->shareResources(now);
  double min_by_net = surf_network_model->shareResources(now);

  XBT_DEBUG("model %p, %s min_by_cpu %f, %s min_by_net %f",
      this, surf_cpu_model_pm->m_name.c_str(), min_by_cpu, surf_network_model->m_name.c_str(), min_by_net);

  if (min_by_cpu >= 0.0 && min_by_net >= 0.0)
    return min(min_by_cpu, min_by_net);
  else if (min_by_cpu >= 0.0)
    return min_by_cpu;
  else if (min_by_net >= 0.0)
    return min_by_net;
  else
    return min_by_cpu;  /* probably min_by_cpu == min_by_net == -1 */
}

void WorkstationModel::updateActionsState(double now, double delta){
  return;
}

ActionPtr WorkstationModel::executeParallelTask(int workstation_nb,
                                        void **workstation_list,
                                        double *computation_amount,
                                        double *communication_amount,
                                        double rate){
#define cost_or_zero(array,pos) ((array)?(array)[pos]:0.0)
  if ((workstation_nb == 1)
      && (cost_or_zero(communication_amount, 0) == 0.0))
    return ((WorkstationCLM03Ptr)workstation_list[0])->execute(computation_amount[0]);
  else if ((workstation_nb == 1)
           && (cost_or_zero(computation_amount, 0) == 0.0))
    return communicate((WorkstationCLM03Ptr)workstation_list[0], (WorkstationCLM03Ptr)workstation_list[0],communication_amount[0], rate);
  else if ((workstation_nb == 2)
             && (cost_or_zero(computation_amount, 0) == 0.0)
             && (cost_or_zero(computation_amount, 1) == 0.0)) {
    int i,nb = 0;
    double value = 0.0;

    for (i = 0; i < workstation_nb * workstation_nb; i++) {
      if (cost_or_zero(communication_amount, i) > 0.0) {
        nb++;
        value = cost_or_zero(communication_amount, i);
      }
    }
    if (nb == 1)
      return communicate((WorkstationCLM03Ptr)workstation_list[0], (WorkstationCLM03Ptr)workstation_list[1],value, rate);
  }
#undef cost_or_zero

  THROW_UNIMPLEMENTED;          /* This model does not implement parallel tasks */
  return NULL;
}

/* returns an array of network_link_CM02_t */
xbt_dynar_t WorkstationModel::getRoute(WorkstationCLM03Ptr src, WorkstationCLM03Ptr dst)
{
  XBT_DEBUG("ws_get_route");
  return surf_network_model->getRoute(src->p_netElm, dst->p_netElm);
}

ActionPtr WorkstationModel::communicate(WorkstationCLM03Ptr src, WorkstationCLM03Ptr dst, double size, double rate){
  return surf_network_model->communicate(src->p_netElm, dst->p_netElm, size, rate);
}



/************
 * Resource *
 ************/
WorkstationCLM03::WorkstationCLM03(WorkstationModelPtr model, const char* name, xbt_dict_t properties, xbt_dynar_t storage, RoutingEdgePtr netElm, CpuPtr cpu)
  : Resource(model, name, properties), p_storage(storage), p_netElm(netElm), p_cpu(cpu) {}

bool WorkstationCLM03::isUsed(){
  THROW_IMPOSSIBLE;             /* This model does not implement parallel tasks */
  return -1;
}

void WorkstationCLM03::updateState(tmgr_trace_event_t event_type, double value, double date){
  THROW_IMPOSSIBLE;             /* This model does not implement parallel tasks */
}

ActionPtr WorkstationCLM03::execute(double size) {
  return p_cpu->execute(size);
}

ActionPtr WorkstationCLM03::sleep(double duration) {
  return p_cpu->sleep(duration);
}

e_surf_resource_state_t WorkstationCLM03::getState() {
  return p_cpu->getState();
}

int WorkstationCLM03::getCore(){
  return p_cpu->getCore();
}

double WorkstationCLM03::getSpeed(double load){
  return p_cpu->getSpeed(load);
}

double WorkstationCLM03::getAvailableSpeed(){
  return p_cpu->getAvailableSpeed();
}

double WorkstationCLM03::getCurrentPowerPeak()
{
  return p_cpu->getCurrentPowerPeak();
}

double WorkstationCLM03::getPowerPeakAt(int pstate_index)
{
  return p_cpu->getPowerPeakAt(pstate_index);
}

int WorkstationCLM03::getNbPstates()
{
  return p_cpu->getNbPstates();
}

void WorkstationCLM03::setPowerPeakAt(int pstate_index)
{
	p_cpu->setPowerPeakAt(pstate_index);
}

double WorkstationCLM03::getConsumedEnergy()
{
  return p_cpu->getConsumedEnergy();
}


xbt_dict_t WorkstationCLM03::getProperties()
{
  return p_cpu->m_properties;
}


StoragePtr WorkstationCLM03::findStorageOnMountList(const char* mount)
{
  StoragePtr st = NULL;
  s_mount_t mnt;
  unsigned int cursor;

  XBT_DEBUG("Search for storage name '%s' on '%s'", mount, m_name);
  xbt_dynar_foreach(p_storage,cursor,mnt)
  {
    XBT_DEBUG("See '%s'",mnt.name);
    if(!strcmp(mount,mnt.name)){
      st = dynamic_cast<StoragePtr>(static_cast<ResourcePtr>(mnt.storage));
      break;
    }
  }
  if(!st) xbt_die("Can't find mount '%s' for '%s'", mount, m_name);
  return st;
}

xbt_dict_t WorkstationCLM03::getStorageList()
{
  s_mount_t mnt;
  unsigned int i;
  xbt_dict_t storage_list = xbt_dict_new_homogeneous(NULL);
  char *storage_name = NULL;

  xbt_dynar_foreach(p_storage,i,mnt){
    storage_name = (char *)dynamic_cast<StoragePtr>(static_cast<ResourcePtr>(mnt.storage))->m_name;
    xbt_dict_set(storage_list,mnt.name,storage_name,NULL);
  }
  return storage_list;
}

ActionPtr WorkstationCLM03::open(const char* mount, const char* path) {
  StoragePtr st = findStorageOnMountList(mount);
  XBT_DEBUG("OPEN on disk '%s'", st->m_name);
  return st->open(mount, path);
}

ActionPtr WorkstationCLM03::close(surf_file_t fd) {
  StoragePtr st = findStorageOnMountList(fd->mount);
  XBT_DEBUG("CLOSE on disk '%s'",st->m_name);
  return st->close(fd);
}

ActionPtr WorkstationCLM03::read(surf_file_t fd, sg_storage_size_t size) {
  StoragePtr st = findStorageOnMountList(fd->mount);
  XBT_DEBUG("READ on disk '%s'",st->m_name);
  return st->read(fd, size);
}

ActionPtr WorkstationCLM03::write(surf_file_t fd, sg_storage_size_t size) {
  StoragePtr st = findStorageOnMountList(fd->mount);
  XBT_DEBUG("WRITE on disk '%s'",st->m_name);
  return st->write(fd, size);
}

int WorkstationCLM03::unlink(surf_file_t fd) {
  if (!fd){
    XBT_WARN("No such file descriptor. Impossible to unlink");
    return 0;
  } else {
//    XBT_INFO("%s %zu", fd->storage, fd->size);
    StoragePtr st = findStorageOnMountList(fd->mount);
    /* Check if the file is on this storage */
    if (!xbt_dict_get_or_null(st->p_content, fd->name)){
      XBT_WARN("File %s is not on disk %s. Impossible to unlink", fd->name,
          st->m_name);
      return 0;
    } else {
      XBT_DEBUG("UNLINK on disk '%s'",st->m_name);
      st->m_usedSize -= fd->size;

      // Remove the file from storage
      xbt_dict_remove(st->p_content, fd->name);

      free(fd->name);
      free(fd->mount);
      xbt_free(fd);
      return 1;
    }
  }
}

ActionPtr WorkstationCLM03::ls(const char* mount, const char *path){
  XBT_DEBUG("LS on mount '%s' and file '%s'", mount, path);
  StoragePtr st = findStorageOnMountList(mount);
  return st->ls(path);
}

sg_storage_size_t WorkstationCLM03::getSize(surf_file_t fd){
  return fd->size;
}

xbt_dynar_t WorkstationCLM03::getInfo( surf_file_t fd)
{
  StoragePtr st = findStorageOnMountList(fd->mount);
  sg_storage_size_t *psize = xbt_new(sg_storage_size_t, 1);
  *psize = fd->size;
  xbt_dynar_t info = xbt_dynar_new(sizeof(void*), NULL);
  xbt_dynar_push_as(info, sg_storage_size_t *, psize);
  xbt_dynar_push_as(info, void *, fd->mount);
  xbt_dynar_push_as(info, void *, (void *)st->m_name);
  xbt_dynar_push_as(info, void *, st->p_typeId);
  xbt_dynar_push_as(info, void *, st->p_contentType);

  return info;
}

sg_storage_size_t WorkstationCLM03::getFreeSize(const char* name)
{
  StoragePtr st = findStorageOnMountList(name);
  return st->m_size - st->m_usedSize;
}

sg_storage_size_t WorkstationCLM03::getUsedSize(const char* name)
{
  StoragePtr st = findStorageOnMountList(name);
  return st->m_usedSize;
}

e_surf_resource_state_t WorkstationCLM03Lmm::getState() {
  return WorkstationCLM03::getState();
}

xbt_dynar_t WorkstationCLM03::getVms()
{
  xbt_dynar_t dyn = xbt_dynar_new(sizeof(smx_host_t), NULL);

  /* iterate for all hosts including virtual machines */
  xbt_lib_cursor_t cursor;
  char *key;
  void **ind_host;
  xbt_lib_foreach(host_lib, cursor, key, ind_host) {
    WorkstationCLM03Ptr ws_clm03 = dynamic_cast<WorkstationCLM03Ptr>(static_cast<ResourcePtr>(ind_host[SURF_WKS_LEVEL]));
    if (!ws_clm03)
      continue;
    /* skip if it is not a virtual machine */
    if (ws_clm03->p_model != static_cast<ModelPtr>(surf_vm_workstation_model))
      continue;

    /* It is a virtual machine, so we can cast it to workstation_VM2013_t */
    WorkstationVM2013Ptr ws_vm2013 = dynamic_cast<WorkstationVM2013Ptr>(ws_clm03);
    if (this == ws_vm2013-> p_subWs)
      xbt_dynar_push(dyn, &ws_vm2013->p_subWs);
  }

  return dyn;
}

void WorkstationCLM03::getParams(ws_params_t params)
{
  memcpy(params, &p_params, sizeof(s_ws_params_t));
}

void WorkstationCLM03::setParams(ws_params_t params)
{
  /* may check something here. */
  memcpy(&p_params, params, sizeof(s_ws_params_t));
}
/**********
 * Action *
 **********/
