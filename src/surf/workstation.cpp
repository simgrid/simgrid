#include "workstation.hpp"
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
  xbt_cfg_setdefault_boolean(_sg_cfg_set, "network/crosstraffic", xbt_strdup("yes"));
  surf_cpu_model_init_Cas01();
  surf_network_model_init_LegrandVelho();

  ModelPtr model = static_cast<ModelPtr>(surf_workstation_model);
  xbt_dynar_push(model_list, &model);
  sg_platf_host_add_cb(workstation_new);
}

void surf_workstation_model_init_compound()
{

  xbt_assert(surf_cpu_model, "No CPU model defined yet!");
  xbt_assert(surf_network_model, "No network model defined yet!");
  surf_workstation_model = new WorkstationModel();

  ModelPtr model = static_cast<ModelPtr>(surf_workstation_model);
  xbt_dynar_push(model_list, &model);
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

double WorkstationModel::shareResources(double now){
  return -1.0;
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

xbt_dict_t WorkstationCLM03::getProperties()
{
  return p_cpu->m_properties;
}


StoragePtr WorkstationCLM03::findStorageOnMountList(const char* storage)
{
  StoragePtr st = NULL;
  s_mount_t mnt;
  unsigned int cursor;

  XBT_DEBUG("Search for storage name '%s' on '%s'",storage,m_name);
  xbt_dynar_foreach(p_storage,cursor,mnt)
  {
    XBT_DEBUG("See '%s'",mnt.name);
    if(!strcmp(storage,mnt.name)){
      st = (StoragePtr)mnt.id;
      break;
    }
  }
  if(!st) xbt_die("Can't find mount '%s' for '%s'",storage,m_name);
  return st;
}

ActionPtr WorkstationCLM03::open(const char* mount, const char* path) {
  StoragePtr st = findStorageOnMountList(mount);
  XBT_DEBUG("OPEN on disk '%s'", st->m_name);
  return st->open(mount, path);
}

ActionPtr WorkstationCLM03::close(surf_file_t fd) {
  StoragePtr st = findStorageOnMountList(fd->storage);
  XBT_DEBUG("CLOSE on disk '%s'",st->m_name);
  return st->close(fd);
}

ActionPtr WorkstationCLM03::read(void* ptr, size_t size, surf_file_t fd) {
  StoragePtr st = findStorageOnMountList(fd->storage);
  XBT_DEBUG("READ on disk '%s'",st->m_name);
  return st->read(ptr, size, fd);
}

ActionPtr WorkstationCLM03::write(const void* ptr, size_t size, surf_file_t fd) {
  StoragePtr st = findStorageOnMountList(fd->storage);
  XBT_DEBUG("WRITE on disk '%s'",st->m_name);
  return st->write(ptr, size, fd);
}

int WorkstationCLM03::unlink(surf_file_t fd) {
  if (!fd){
    XBT_WARN("No such file descriptor. Impossible to unlink");
    return 0;
  } else {
//    XBT_INFO("%s %zu", fd->storage, fd->size);
    StoragePtr st = findStorageOnMountList(fd->storage);
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
      free(fd->storage);
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

size_t WorkstationCLM03::getSize(surf_file_t fd){
  return fd->size;
}

e_surf_resource_state_t WorkstationCLM03Lmm::getState() {
  return WorkstationCLM03::getState();
}
/**********
 * Action *
 **********/
