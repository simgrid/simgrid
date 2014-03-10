/* Copyright (c) 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "workstation_interface.hpp"
#include "vm_workstation_interface.hpp"
#include "cpu_cas01.hpp"
#include "simgrid/sg_config.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_workstation, surf,
                                "Logging specific to the SURF workstation module");

WorkstationModelPtr surf_workstation_model = NULL;

/*************
 * Callbacks *
 *************/

surf_callback(void, WorkstationPtr) workstationCreatedCallbacks;
surf_callback(void, WorkstationPtr) workstationDestructedCallbacks;
surf_callback(void, WorkstationPtr) workstationStateChangedCallbacks;
surf_callback(void, WorkstationActionPtr) workstationActionStateChangedCallbacks;

/*********
 * Model *
 *********/
WorkstationModel::WorkstationModel(const char *name)
 : Model(name)
{
  p_cpuModel = surf_cpu_model_pm;
}

WorkstationModel::WorkstationModel()
: Model("Workstation") {
  p_cpuModel = surf_cpu_model_pm;
}

WorkstationModel::~WorkstationModel() {
}

/* Each VM has a dummy CPU action on the PM layer. This CPU action works as the
 * constraint (capacity) of the VM in the PM layer. If the VM does not have any
 * active task, the dummy CPU action must be deactivated, so that the VM does
 * not get any CPU share in the PM layer. */
void WorkstationModel::adjustWeightOfDummyCpuActions()
{
  /* iterate for all virtual machines */
  for (WorkstationVMModel::vm_list_t::iterator iter =
         WorkstationVMModel::ws_vms.begin();
       iter !=  WorkstationVMModel::ws_vms.end(); ++iter) {

    WorkstationVMPtr ws_vm = &*iter;
    CpuCas01Ptr cpu_cas01 = static_cast<CpuCas01Ptr>(ws_vm->p_cpu);
    xbt_assert(cpu_cas01, "cpu-less workstation");

    int is_active = lmm_constraint_used(cpu_cas01->getModel()->getMaxminSystem(), cpu_cas01->getConstraint());
    // int is_active_old = constraint_is_active(cpu_cas01);

    // {
    //   xbt_assert(is_active == is_active_old, "%d %d", is_active, is_active_old);
    // }

    if (is_active) {
      /* some tasks exist on this VM */
      XBT_DEBUG("set the weight of the dummy CPU action on PM to 1");

      /* FIXME: we shoud use lmm_update_variable_weight() ? */
      /* FIXME: If we assgign 1.05 and 0.05, the system makes apparently wrong values. */
      ws_vm->p_action->setPriority(1);

    } else {
      /* no task exits on this VM */
      XBT_DEBUG("set the weight of the dummy CPU action on PM to 0");

      ws_vm->p_action->setPriority(0);
    }
  }
}

/************
 * Resource *
 ************/
Workstation::Workstation()
{
  surf_callback_emit(workstationCreatedCallbacks, this);
}

Workstation::Workstation(ModelPtr model, const char *name, xbt_dict_t props,
		                 xbt_dynar_t storage, RoutingEdgePtr netElm, CpuPtr cpu)
 : Resource(model, name, props)
 , p_storage(storage), p_netElm(netElm), p_cpu(cpu)
{
  p_params.ramsize = 0;
  surf_callback_emit(workstationCreatedCallbacks, this);
}

Workstation::Workstation(ModelPtr model, const char *name, xbt_dict_t props, lmm_constraint_t constraint,
				         xbt_dynar_t storage, RoutingEdgePtr netElm, CpuPtr cpu)
 : Resource(model, name, props, constraint)
 , p_storage(storage), p_netElm(netElm), p_cpu(cpu)
{
  p_params.ramsize = 0;
  surf_callback_emit(workstationCreatedCallbacks, this);
}

Workstation::~Workstation(){
  surf_callback_emit(workstationDestructedCallbacks, this);
}

void Workstation::setState(e_surf_resource_state_t state){
  Resource::setState(state);
  surf_callback_emit(workstationStateChangedCallbacks, this);
}

int Workstation::getCore(){
  return p_cpu->getCore();
}

double Workstation::getSpeed(double load){
  return p_cpu->getSpeed(load);
}

double Workstation::getAvailableSpeed(){
  return p_cpu->getAvailableSpeed();
}

double Workstation::getCurrentPowerPeak()
{
  return p_cpu->getCurrentPowerPeak();
}

double Workstation::getPowerPeakAt(int pstate_index)
{
  return p_cpu->getPowerPeakAt(pstate_index);
}

int Workstation::getNbPstates()
{
  return p_cpu->getNbPstates();
}

void Workstation::setPowerPeakAt(int pstate_index)
{
	p_cpu->setPowerPeakAt(pstate_index);
}

xbt_dict_t Workstation::getProperties()
{
  return p_cpu->getProperties();
}

StoragePtr Workstation::findStorageOnMountList(const char* mount)
{
  StoragePtr st = NULL;
  s_mount_t mnt;
  unsigned int cursor;

  XBT_DEBUG("Search for storage name '%s' on '%s'", mount, getName());
  xbt_dynar_foreach(p_storage,cursor,mnt)
  {
    XBT_DEBUG("See '%s'",mnt.name);
    if(!strcmp(mount,mnt.name)){
      st = static_cast<StoragePtr>(mnt.storage);
      break;
    }
  }
  if(!st) xbt_die("Can't find mount '%s' for '%s'", mount, getName());
  return st;
}

xbt_dict_t Workstation::getMountedStorageList()
{
  s_mount_t mnt;
  unsigned int i;
  xbt_dict_t storage_list = xbt_dict_new_homogeneous(NULL);
  char *storage_name = NULL;

  xbt_dynar_foreach(p_storage,i,mnt){
    storage_name = (char *)static_cast<StoragePtr>(mnt.storage)->getName();
    xbt_dict_set(storage_list,mnt.name,storage_name,NULL);
  }
  return storage_list;
}

xbt_dynar_t Workstation::getAttachedStorageList()
{
  xbt_lib_cursor_t cursor;
  char *key;
  void **data;
  xbt_dynar_t result = xbt_dynar_new(sizeof(void*), NULL);
  xbt_lib_foreach(storage_lib, cursor, key, data) {
	  if(xbt_lib_get_level(xbt_lib_get_elm_or_null(storage_lib, key), SURF_STORAGE_LEVEL) != NULL) {
	    StoragePtr storage = static_cast<StoragePtr>(xbt_lib_get_level(xbt_lib_get_elm_or_null(storage_lib, key), SURF_STORAGE_LEVEL));
	    if(!strcmp((const char*)storage->p_attach,this->getName())){
	      xbt_dynar_push_as(result, void *,(void *)static_cast<ResourcePtr>(storage)->getName());
	    }
	  }
  }
  return result;
}

ActionPtr Workstation::open(const char* mount, const char* path) {
  StoragePtr st = findStorageOnMountList(mount);
  XBT_DEBUG("OPEN on disk '%s'", st->getName());
  return st->open(mount, path);
}

ActionPtr Workstation::close(surf_file_t fd) {
  StoragePtr st = findStorageOnMountList(fd->mount);
  XBT_DEBUG("CLOSE on disk '%s'",st->getName());
  return st->close(fd);
}

ActionPtr Workstation::read(surf_file_t fd, sg_size_t size) {
  StoragePtr st = findStorageOnMountList(fd->mount);
  XBT_DEBUG("READ on disk '%s'",st->getName());
  return st->read(fd, size);
}

ActionPtr Workstation::write(surf_file_t fd, sg_size_t size) {
  StoragePtr st = findStorageOnMountList(fd->mount);
  XBT_DEBUG("WRITE on disk '%s'",st->getName());
  return st->write(fd, size);
}

int Workstation::unlink(surf_file_t fd) {
  if (!fd){
    XBT_WARN("No such file descriptor. Impossible to unlink");
    return 0;
  } else {
//    XBT_INFO("%s %zu", fd->storage, fd->size);
    StoragePtr st = findStorageOnMountList(fd->mount);
    /* Check if the file is on this storage */
    if (!xbt_dict_get_or_null(st->p_content, fd->name)){
      XBT_WARN("File %s is not on disk %s. Impossible to unlink", fd->name,
          st->getName());
      return 0;
    } else {
      XBT_DEBUG("UNLINK on disk '%s'",st->getName());
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

ActionPtr Workstation::ls(const char* mount, const char *path){
  XBT_DEBUG("LS on mount '%s' and file '%s'", mount, path);
  StoragePtr st = findStorageOnMountList(mount);
  return st->ls(path);
}

sg_size_t Workstation::getSize(surf_file_t fd){
  return fd->size;
}

xbt_dynar_t Workstation::getInfo( surf_file_t fd)
{
  StoragePtr st = findStorageOnMountList(fd->mount);
  sg_size_t *psize = xbt_new(sg_size_t, 1);
  *psize = fd->size;
  xbt_dynar_t info = xbt_dynar_new(sizeof(void*), NULL);
  xbt_dynar_push_as(info, sg_size_t *, psize);
  xbt_dynar_push_as(info, void *, fd->mount);
  xbt_dynar_push_as(info, void *, (void *)st->getName());
  xbt_dynar_push_as(info, void *, st->p_typeId);
  xbt_dynar_push_as(info, void *, st->p_contentType);

  return info;
}

sg_size_t Workstation::fileTell(surf_file_t fd){
  return fd->current_position;
}

int Workstation::fileSeek(surf_file_t fd, sg_size_t offset, int origin){

  switch (origin) {
  case SEEK_SET:
    fd->current_position = 0;
	return MSG_OK;
  case SEEK_CUR:
	if(offset > fd->size)
	  offset = fd->size;
	fd->current_position = offset;
	return MSG_OK;
  case SEEK_END:
	fd->current_position = fd->size;
	return MSG_OK;
  default:
	return MSG_TASK_CANCELED;
  }
}

sg_size_t Workstation::getFreeSize(const char* name)
{
  StoragePtr st = findStorageOnMountList(name);
  return st->m_size - st->m_usedSize;
}

sg_size_t Workstation::getUsedSize(const char* name)
{
  StoragePtr st = findStorageOnMountList(name);
  return st->m_usedSize;
}

xbt_dynar_t Workstation::getVms()
{
  xbt_dynar_t dyn = xbt_dynar_new(sizeof(smx_host_t), NULL);

  /* iterate for all virtual machines */
  for (WorkstationVMModel::vm_list_t::iterator iter =
         WorkstationVMModel::ws_vms.begin();
       iter !=  WorkstationVMModel::ws_vms.end(); ++iter) {

    WorkstationVMPtr ws_vm = &*iter;
    if (this == ws_vm-> p_subWs)
      xbt_dynar_push(dyn, &ws_vm->p_subWs);
  }

  return dyn;
}

void Workstation::getParams(ws_params_t params)
{
  *params = p_params;
}

void Workstation::setParams(ws_params_t params)
{
  /* may check something here. */
  p_params = *params;
}

/**********
 * Action *
 **********/

void WorkstationAction::setState(e_surf_action_state_t state){
  Action::setState(state);
  surf_callback_emit(workstationActionStateChangedCallbacks, this);
}
