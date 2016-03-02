/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/host.hpp>

#include "src/simix/smx_private.h"
#include "cpu_cas01.hpp"
#include "src/surf/HostImpl.hpp"
#include "simgrid/sg_config.h"

#include "network_interface.hpp"
#include "virtual_machine.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_host, surf,
                                "Logging specific to the SURF host module");

simgrid::surf::HostModel *surf_host_model = NULL;

/*************
 * Callbacks *
 *************/

namespace simgrid {
namespace surf {

simgrid::xbt::Extension<simgrid::s4u::Host, HostImpl> HostImpl::EXTENSION_ID;

/*********
 * Model *
 *********/
HostImpl *HostModel::createHost(const char *name,NetCard *netElm, Cpu *cpu, xbt_dict_t props){
  xbt_dynar_t storageList = (xbt_dynar_t)xbt_lib_get_or_null(storage_lib, name, ROUTING_STORAGE_HOST_LEVEL);

  HostImpl *host = new simgrid::surf::HostImpl(surf_host_model, name, props, storageList, cpu);
  XBT_DEBUG("Create host %s with %ld mounted disks", name, xbt_dynar_length(host->p_storage));
  return host;
}

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
    Cpu *cpu = ws_vm->p_cpu;

    int is_active = lmm_constraint_used(cpu->getModel()->getMaxminSystem(), cpu->getConstraint());

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

Action *HostModel::executeParallelTask(int host_nb,
    sg_host_t *host_list,
    double *flops_amount,
    double *bytes_amount,
    double rate){
#define cost_or_zero(array,pos) ((array)?(array)[pos]:0.0)
  Action *action =NULL;
  if ((host_nb == 1)
      && (cost_or_zero(bytes_amount, 0) == 0.0)){
    action = host_list[0]->pimpl_cpu->execution_start(flops_amount[0]);
  } else if ((host_nb == 1)
           && (cost_or_zero(flops_amount, 0) == 0.0)) {
    action = surf_network_model->communicate(host_list[0]->pimpl_netcard,
                                         host_list[0]->pimpl_netcard,
                       bytes_amount[0], rate);
  } else if ((host_nb == 2)
             && (cost_or_zero(flops_amount, 0) == 0.0)
             && (cost_or_zero(flops_amount, 1) == 0.0)) {
    int i,nb = 0;
    double value = 0.0;

    for (i = 0; i < host_nb * host_nb; i++) {
      if (cost_or_zero(bytes_amount, i) > 0.0) {
        nb++;
        value = cost_or_zero(bytes_amount, i);
      }
    }
    if (nb == 1){
      action = surf_network_model->communicate(host_list[0]->pimpl_netcard,
                                           host_list[1]->pimpl_netcard,
                         value, rate);
    }
  } else
    THROW_UNIMPLEMENTED;      /* This model does not implement parallel tasks for more than 2 hosts */
#undef cost_or_zero
  xbt_free(host_list);
  return action;
}

/************
 * Resource *
 ************/


void HostImpl::classInit()
{
  if (!EXTENSION_ID.valid()) {
    EXTENSION_ID = simgrid::s4u::Host::extension_create<simgrid::surf::HostImpl>();
  }
}

HostImpl::HostImpl(simgrid::surf::HostModel *model, const char *name, xbt_dict_t props,
                     xbt_dynar_t storage, Cpu *cpu)
 : Resource(model, name)
 , PropertyHolder(props)
 , p_storage(storage), p_cpu(cpu)
{
  p_params.ramsize = 0;
}

HostImpl::HostImpl(simgrid::surf::HostModel *model, const char *name, xbt_dict_t props, lmm_constraint_t constraint,
                 xbt_dynar_t storage, Cpu *cpu)
 : Resource(model, name, constraint)
 , PropertyHolder(props)
 , p_storage(storage), p_cpu(cpu)
{
  p_params.ramsize = 0;
}

/** @brief use destroy() instead of this destructor */
HostImpl::~HostImpl()
{
}

void HostImpl::attach(simgrid::s4u::Host* host)
{
  if (p_host != nullptr)
    xbt_die("Already attached to host %s", host->name().c_str());
  host->extension_set(this);
  p_host = host;
}

bool HostImpl::isOn() {
  return p_cpu->isOn();
}
bool HostImpl::isOff() {
  return p_cpu->isOff();
}
void HostImpl::turnOn(){
  if (isOff()) {
    p_cpu->turnOn();
    simgrid::s4u::Host::onStateChange(*this->p_host);
  }
}
void HostImpl::turnOff(){
  if (isOn()) {
    p_cpu->turnOff();
    simgrid::s4u::Host::onStateChange(*this->p_host);
  }
}

simgrid::surf::Storage *HostImpl::findStorageOnMountList(const char* mount)
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

xbt_dict_t HostImpl::getMountedStorageList()
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

xbt_dynar_t HostImpl::getAttachedStorageList()
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

Action *HostImpl::open(const char* fullpath) {

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

Action *HostImpl::close(surf_file_t fd) {
  simgrid::surf::Storage *st = findStorageOnMountList(fd->mount);
  XBT_DEBUG("CLOSE %s on disk '%s'",fd->name, st->getName());
  return st->close(fd);
}

Action *HostImpl::read(surf_file_t fd, sg_size_t size) {
  simgrid::surf::Storage *st = findStorageOnMountList(fd->mount);
  XBT_DEBUG("READ %s on disk '%s'",fd->name, st->getName());
  return st->read(fd, size);
}

Action *HostImpl::write(surf_file_t fd, sg_size_t size) {
  simgrid::surf::Storage *st = findStorageOnMountList(fd->mount);
  XBT_DEBUG("WRITE %s on disk '%s'",fd->name, st->getName());
  return st->write(fd, size);
}

int HostImpl::unlink(surf_file_t fd) {
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

sg_size_t HostImpl::getSize(surf_file_t fd){
  return fd->size;
}

xbt_dynar_t HostImpl::getInfo( surf_file_t fd)
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

sg_size_t HostImpl::fileTell(surf_file_t fd){
  return fd->current_position;
}

int HostImpl::fileSeek(surf_file_t fd, sg_offset_t offset, int origin){

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

int HostImpl::fileMove(surf_file_t fd, const char* fullpath){
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

xbt_dynar_t HostImpl::getVms()
{
  xbt_dynar_t dyn = xbt_dynar_new(sizeof(simgrid::surf::VirtualMachine*), NULL);

  /* iterate for all virtual machines */
  for (simgrid::surf::VMModel::vm_list_t::iterator iter =
         simgrid::surf::VMModel::ws_vms.begin();
       iter !=  simgrid::surf::VMModel::ws_vms.end(); ++iter) {

    simgrid::surf::VirtualMachine *ws_vm = &*iter;
    if (this == ws_vm->p_hostPM->extension(simgrid::surf::HostImpl::EXTENSION_ID))
      xbt_dynar_push(dyn, &ws_vm);
  }

  return dyn;
}

void HostImpl::getParams(vm_params_t params)
{
  *params = p_params;
}

void HostImpl::setParams(vm_params_t params)
{
  /* may check something here. */
  p_params = *params;
}

}
}
