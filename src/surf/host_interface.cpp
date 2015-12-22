/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "host_interface.hpp"

#include <simgrid/Host.hpp>

#include "src/simix/smx_private.h"
#include "cpu_cas01.hpp"
#include "simgrid/sg_config.h"

#include "network_interface.hpp"
#include "virtual_machine.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_host, surf,
                                "Logging specific to the SURF host module");

simgrid::surf::HostModel *surf_host_model = NULL;

void host_add_traces(){
  surf_host_model->addTraces();
}

/*************
 * Callbacks *
 *************/

namespace simgrid {
namespace surf {

simgrid::xbt::Extension<simgrid::Host, Host> Host::EXTENSION_ID;

/*********
 * Model *
 *********/

/* Each VM has a dummy CPU action on the PM layer. This CPU action works as the
 * constraint (capacity) of the VM in the PM layer. If the VM does not have any
 * active task, the dummy CPU action must be deactivated, so that the VM does
 * not get any CPU share in the PM layer. */
void HostModel::adjustWeightOfDummyCpuActions()
{
  /* iterate for all virtual machines */
  for (VMModel::vm_list_t::iterator iter =
         VMModel::ws_vms.begin();
       iter !=  VMModel::ws_vms.end(); ++iter) {

    VirtualMachine *ws_vm = &*iter;
    CpuCas01 *cpu_cas01 = static_cast<CpuCas01*>(ws_vm->p_cpu);
    xbt_assert(cpu_cas01, "cpu-less host");

    int is_active = lmm_constraint_used(cpu_cas01->getModel()->getMaxminSystem(), cpu_cas01->getConstraint());
    // int is_active_old = constraint_is_active(cpu_cas01);

    if (is_active) {
      /* some tasks exist on this VM */
      XBT_DEBUG("set the weight of the dummy CPU action on PM to 1");

      /* FIXME: we should use lmm_update_variable_weight() ? */
      /* FIXME: If we assign 1.05 and 0.05, the system makes apparently wrong values. */
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
simgrid::surf::signal<void(simgrid::surf::Host*)> Host::onCreation;
simgrid::surf::signal<void(simgrid::surf::Host*)> Host::onDestruction;
simgrid::surf::signal<void(simgrid::surf::Host*, e_surf_resource_state_t, e_surf_resource_state_t)> Host::onStateChange;

static void host_destroy(void *h){
	static_cast<simgrid::surf::Host*>(h)->destroy();
}

void Host::classInit()
{
  if (!EXTENSION_ID.valid()) {
    EXTENSION_ID = simgrid::Host::extension_create<simgrid::surf::Host>(host_destroy);
    SURF_HOST_LEVEL = EXTENSION_ID.id(); // FIXME: KILLME
  }
}

Host::Host(simgrid::surf::Model *model, const char *name, xbt_dict_t props,
		                 xbt_dynar_t storage, RoutingEdge *netElm, Cpu *cpu)
 : Resource(model, name)
 , PropertyHolder(props)
 , p_storage(storage), p_netElm(netElm), p_cpu(cpu)
{
  p_params.ramsize = 0;
}

Host::Host(simgrid::surf::Model *model, const char *name, xbt_dict_t props, lmm_constraint_t constraint,
				         xbt_dynar_t storage, RoutingEdge *netElm, Cpu *cpu)
 : Resource(model, name, constraint)
 , PropertyHolder(props)
 , p_storage(storage), p_netElm(netElm), p_cpu(cpu)
{
  p_params.ramsize = 0;
}

/** @brief use destroy() instead of this destructor */
Host::~Host()
{
	xbt_assert(currentlyDestroying_, "Don't delete Hosts directly. Call destroy() instead.");
}
/** @brief Fire the require callbacks and destroy the object
 *
 * Don't delete directly an Host, call h->destroy() instead.
 */
void Host::destroy()
{
	if (!currentlyDestroying_) {
		currentlyDestroying_ = true;
		onDestruction(this);
		delete this;
	}
}

void Host::attach(simgrid::Host* host)
{
  if (p_host != nullptr)
    xbt_die("Already attached to host %s", host->id().c_str());
  host->extension_set(this);
  p_host = host;
  onCreation(this);
}

e_surf_resource_state_t Host::getState() {
  return p_cpu->getState();
}
void Host::setState(e_surf_resource_state_t state){
  e_surf_resource_state_t old = Resource::getState();
  Resource::setState(state);
  onStateChange(this, old, state);
  p_cpu->setState(state);
}

Action *Host::execute(double size) {
  return p_cpu->execute(size);
}
Action *Host::sleep(double duration) {
  return p_cpu->sleep(duration);
}


simgrid::surf::Storage *Host::findStorageOnMountList(const char* mount)
{
  simgrid::surf::Storage *st = NULL;
  s_mount_t mnt;
  unsigned int cursor;

  XBT_DEBUG("Search for storage name '%s' on '%s'", mount, getName());
  xbt_dynar_foreach(p_storage,cursor,mnt)
  {
    XBT_DEBUG("See '%s'",mnt.name);
    if(!strcmp(mount,mnt.name)){
      st = static_cast<simgrid::surf::Storage*>(mnt.storage);
      break;
    }
  }
  if(!st) xbt_die("Can't find mount '%s' for '%s'", mount, getName());
  return st;
}

xbt_dict_t Host::getMountedStorageList()
{
  s_mount_t mnt;
  unsigned int i;
  xbt_dict_t storage_list = xbt_dict_new_homogeneous(NULL);
  char *storage_name = NULL;

  xbt_dynar_foreach(p_storage,i,mnt){
    storage_name = (char *)static_cast<simgrid::surf::Storage*>(mnt.storage)->getName();
    xbt_dict_set(storage_list,mnt.name,storage_name,NULL);
  }
  return storage_list;
}

xbt_dynar_t Host::getAttachedStorageList()
{
  xbt_lib_cursor_t cursor;
  char *key;
  void **data;
  xbt_dynar_t result = xbt_dynar_new(sizeof(void*), NULL);
  xbt_lib_foreach(storage_lib, cursor, key, data) {
    if(xbt_lib_get_level(xbt_lib_get_elm_or_null(storage_lib, key), SURF_STORAGE_LEVEL) != NULL) {
	  simgrid::surf::Storage *storage = static_cast<simgrid::surf::Storage*>(xbt_lib_get_level(xbt_lib_get_elm_or_null(storage_lib, key), SURF_STORAGE_LEVEL));
	  if(!strcmp((const char*)storage->p_attach,this->getName())){
	    xbt_dynar_push_as(result, void *, (void*)storage->getName());
	  }
	}
  }
  return result;
}

Action *Host::open(const char* fullpath) {

  simgrid::surf::Storage *st = NULL;
  s_mount_t mnt;
  unsigned int cursor;
  size_t longest_prefix_length = 0;
  char *path = NULL;
  char *file_mount_name = NULL;
  char *mount_name = NULL;

  XBT_DEBUG("Search for storage name for '%s' on '%s'", fullpath, getName());
  xbt_dynar_foreach(p_storage,cursor,mnt)
  {
    XBT_DEBUG("See '%s'",mnt.name);
    file_mount_name = (char *) xbt_malloc ((strlen(mnt.name)+1));
    strncpy(file_mount_name,fullpath,strlen(mnt.name)+1);
    file_mount_name[strlen(mnt.name)] = '\0';

    if(!strcmp(file_mount_name,mnt.name) && strlen(mnt.name)>longest_prefix_length)
    {/* The current mount name is found in the full path and is bigger than the previous*/
      longest_prefix_length = strlen(mnt.name);
      st = static_cast<simgrid::surf::Storage*>(mnt.storage);
    }
    free(file_mount_name);
  }
  if(longest_prefix_length>0)
  { /* Mount point found, split fullpath into mount_name and path+filename*/
	path = (char *) xbt_malloc ((strlen(fullpath)-longest_prefix_length+1));
	mount_name = (char *) xbt_malloc ((longest_prefix_length+1));
	strncpy(mount_name, fullpath, longest_prefix_length+1);
	strncpy(path, fullpath+longest_prefix_length, strlen(fullpath)-longest_prefix_length+1);
	path[strlen(fullpath)-longest_prefix_length] = '\0';
	mount_name[longest_prefix_length] = '\0';
  }
  else
    xbt_die("Can't find mount point for '%s' on '%s'", fullpath, getName());

  XBT_DEBUG("OPEN %s on disk '%s'",path, st->getName());
  Action *action = st->open((const char*)mount_name, (const char*)path);
  free((char*)path);
  free((char*)mount_name);
  return action;
}

Action *Host::close(surf_file_t fd) {
  simgrid::surf::Storage *st = findStorageOnMountList(fd->mount);
  XBT_DEBUG("CLOSE %s on disk '%s'",fd->name, st->getName());
  return st->close(fd);
}

Action *Host::read(surf_file_t fd, sg_size_t size) {
  simgrid::surf::Storage *st = findStorageOnMountList(fd->mount);
  XBT_DEBUG("READ %s on disk '%s'",fd->name, st->getName());
  return st->read(fd, size);
}

Action *Host::write(surf_file_t fd, sg_size_t size) {
  simgrid::surf::Storage *st = findStorageOnMountList(fd->mount);
  XBT_DEBUG("WRITE %s on disk '%s'",fd->name, st->getName());
  return st->write(fd, size);
}

int Host::unlink(surf_file_t fd) {
  if (!fd){
    XBT_WARN("No such file descriptor. Impossible to unlink");
    return -1;
  } else {

    simgrid::surf::Storage *st = findStorageOnMountList(fd->mount);
    /* Check if the file is on this storage */
    if (!xbt_dict_get_or_null(st->p_content, fd->name)){
      XBT_WARN("File %s is not on disk %s. Impossible to unlink", fd->name,
          st->getName());
      return -1;
    } else {
      XBT_DEBUG("UNLINK %s on disk '%s'",fd->name, st->getName());
      st->m_usedSize -= fd->size;

      // Remove the file from storage
      xbt_dict_remove(st->p_content, fd->name);

      xbt_free(fd->name);
      xbt_free(fd->mount);
      xbt_free(fd);
      return 0;
    }
  }
}

sg_size_t Host::getSize(surf_file_t fd){
  return fd->size;
}

xbt_dynar_t Host::getInfo( surf_file_t fd)
{
  simgrid::surf::Storage *st = findStorageOnMountList(fd->mount);
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

sg_size_t Host::fileTell(surf_file_t fd){
  return fd->current_position;
}

int Host::fileSeek(surf_file_t fd, sg_offset_t offset, int origin){

  switch (origin) {
  case SEEK_SET:
    fd->current_position = offset;
    return 0;
  case SEEK_CUR:
    fd->current_position += offset;
    return 0;
  case SEEK_END:
    fd->current_position = fd->size + offset;
    return 0;
  default:
    return -1;
  }
}

int Host::fileMove(surf_file_t fd, const char* fullpath){
  /* Check if the new full path is on the same mount point */
  if(!strncmp((const char*)fd->mount, fullpath, strlen(fd->mount))) {
    sg_size_t *psize, *new_psize;
    psize = (sg_size_t*)
        xbt_dict_get_or_null(findStorageOnMountList(fd->mount)->p_content,
                             fd->name);
    new_psize = xbt_new(sg_size_t, 1);
    *new_psize = *psize;
    if (psize){// src file exists
      xbt_dict_remove(findStorageOnMountList(fd->mount)->p_content, fd->name);
      char *path = (char *) xbt_malloc ((strlen(fullpath)-strlen(fd->mount)+1));
      strncpy(path, fullpath+strlen(fd->mount),
              strlen(fullpath)-strlen(fd->mount)+1);
      xbt_dict_set(findStorageOnMountList(fd->mount)->p_content, path,
                   new_psize,NULL);
      XBT_DEBUG("Move file from %s to %s, size '%llu'",fd->name, fullpath, *psize);
      free(path);
      return 0;
    } else {
      XBT_WARN("File %s doesn't exist", fd->name);
      return -1;
    }
  } else {
    XBT_WARN("New full path %s is not on the same mount point: %s. Action has been canceled.",
             fullpath, fd->mount);
    return -1;
  }
}

xbt_dynar_t Host::getVms()
{
  xbt_dynar_t dyn = xbt_dynar_new(sizeof(simgrid::surf::VirtualMachine*), NULL);

  /* iterate for all virtual machines */
  for (simgrid::surf::VMModel::vm_list_t::iterator iter =
         simgrid::surf::VMModel::ws_vms.begin();
       iter !=  simgrid::surf::VMModel::ws_vms.end(); ++iter) {

    simgrid::surf::VirtualMachine *ws_vm = &*iter;
    if (this == ws_vm->p_subWs)
      xbt_dynar_push(dyn, &ws_vm);
  }

  return dyn;
}

void Host::getParams(vm_params_t params)
{
  *params = p_params;
}

void Host::setParams(vm_params_t params)
{
  /* may check something here. */
  p_params = *params;
}

}
}
