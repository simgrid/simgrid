/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/host.hpp>

#include "src/simix/smx_private.h"
#include "cpu_cas01.hpp"
#include "src/surf/HostImpl.hpp"
#include "simgrid/sg_config.h"

#include "VirtualMachineImpl.hpp"
#include "network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_host, surf, "Logging specific to the SURF host module");

simgrid::surf::HostModel *surf_host_model = nullptr;

/*************
 * Callbacks *
 *************/

namespace simgrid {
namespace surf {

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
  for (VirtualMachineImpl* ws_vm : VirtualMachineImpl::allVms_) {

    Cpu* cpu = ws_vm->piface_->pimpl_cpu;

    int is_active = lmm_constraint_used(cpu->getModel()->getMaxminSystem(), cpu->getConstraint());

    if (is_active) {
      /* some tasks exist on this VM */
      XBT_DEBUG("set the weight of the dummy CPU action on PM to 1");

      /* FIXME: we should use lmm_update_variable_weight() ? */
      /* FIXME: If we assign 1.05 and 0.05, the system makes apparently wrong values. */
      ws_vm->action_->setPriority(1);

    } else {
      /* no task exits on this VM */
      XBT_DEBUG("set the weight of the dummy CPU action on PM to 0");

      ws_vm->action_->setPriority(0);
    }
  }
}

Action* HostModel::executeParallelTask(int host_nb, simgrid::s4u::Host** host_list, double* flops_amount,
                                       double* bytes_amount, double rate)
{
#define cost_or_zero(array,pos) ((array)?(array)[pos]:0.0)
  Action* action = nullptr;
  if ((host_nb == 1) && (cost_or_zero(bytes_amount, 0) == 0.0)) {
    action = host_list[0]->pimpl_cpu->execution_start(flops_amount[0]);
  } else if ((host_nb == 1) && (cost_or_zero(flops_amount, 0) == 0.0)) {
    action = surf_network_model->communicate(host_list[0], host_list[0], bytes_amount[0], rate);
  } else if ((host_nb == 2) && (cost_or_zero(flops_amount, 0) == 0.0) && (cost_or_zero(flops_amount, 1) == 0.0)) {
    int i, nb = 0;
    double value = 0.0;

    for (i = 0; i < host_nb * host_nb; i++) {
      if (cost_or_zero(bytes_amount, i) > 0.0) {
        nb++;
        value = cost_or_zero(bytes_amount, i);
      }
    }
    if (nb == 1) {
      action = surf_network_model->communicate(host_list[0], host_list[1], value, rate);
    } else if (nb == 0) {
      xbt_die("Cannot have a communication with no flop to exchange in this model. You should consider using the "
              "ptask model");
    } else {
      xbt_die("Cannot have a communication that is not a simple point-to-point in this model. You should consider "
              "using the ptask model");
    }
  } else
    xbt_die("This model only accepts one of the following. You should consider using the ptask model for the other "
            "cases.\n - execution with one host only and no communication\n - Self-comms with one host only\n - "
            "Communications with two hosts and no computation");
#undef cost_or_zero
  xbt_free(host_list);
  return action;
}

/************
 * Resource *
 ************/
HostImpl::HostImpl(s4u::Host* host, xbt_dynar_t storage) : storage_(storage), piface_(host)
{
  piface_->pimpl_ = this;
}

/** @brief use destroy() instead of this destructor */
HostImpl::~HostImpl() = default;

simgrid::surf::Storage* HostImpl::findStorageOnMountList(const char* mount)
{
  simgrid::surf::Storage* st = nullptr;
  s_mount_t mnt;
  unsigned int cursor;

  XBT_DEBUG("Search for storage name '%s' on '%s'", mount, piface_->name().c_str());
  xbt_dynar_foreach (storage_, cursor, mnt) {
    XBT_DEBUG("See '%s'", mnt.name);
    if (!strcmp(mount, mnt.name)) {
      st = static_cast<simgrid::surf::Storage*>(mnt.storage);
      break;
    }
  }
  if (!st)
    xbt_die("Can't find mount '%s' for '%s'", mount, piface_->name().c_str());
  return st;
}

xbt_dict_t HostImpl::getMountedStorageList()
{
  s_mount_t mnt;
  unsigned int i;
  xbt_dict_t storage_list = xbt_dict_new_homogeneous(nullptr);
  char* storage_name      = nullptr;

  xbt_dynar_foreach (storage_, i, mnt) {
    storage_name = (char*)static_cast<simgrid::surf::Storage*>(mnt.storage)->getName();
    xbt_dict_set(storage_list, mnt.name, storage_name, nullptr);
  }
  return storage_list;
}

xbt_dynar_t HostImpl::getAttachedStorageList()
{
  xbt_lib_cursor_t cursor;
  char* key;
  void** data;
  xbt_dynar_t result = xbt_dynar_new(sizeof(void*), nullptr);
  xbt_lib_foreach(storage_lib, cursor, key, data)
  {
    if (xbt_lib_get_level(xbt_lib_get_elm_or_null(storage_lib, key), SURF_STORAGE_LEVEL) != nullptr) {
      simgrid::surf::Storage* storage = static_cast<simgrid::surf::Storage*>(
          xbt_lib_get_level(xbt_lib_get_elm_or_null(storage_lib, key), SURF_STORAGE_LEVEL));
      if (!strcmp((const char*)storage->attach_, piface_->name().c_str())) {
        xbt_dynar_push_as(result, void*, (void*)storage->getName());
      }
    }
  }
  return result;
    }

    Action* HostImpl::open(const char* fullpath)
    {

      simgrid::surf::Storage* st = nullptr;
      s_mount_t mnt;
      unsigned int cursor;
      size_t longest_prefix_length = 0;
      char* path                   = nullptr;
      char* file_mount_name        = nullptr;
      char* mount_name             = nullptr;

      XBT_DEBUG("Search for storage name for '%s' on '%s'", fullpath, piface_->name().c_str());
      xbt_dynar_foreach (storage_, cursor, mnt) {
        XBT_DEBUG("See '%s'", mnt.name);
        file_mount_name = (char*)xbt_malloc((strlen(mnt.name) + 1));
        strncpy(file_mount_name, fullpath, strlen(mnt.name) + 1);
        file_mount_name[strlen(mnt.name)] = '\0';

        if (!strcmp(file_mount_name, mnt.name) &&
            strlen(mnt.name) > longest_prefix_length) { /* The current mount name is found in the full path and is
                                                           bigger than the previous*/
          longest_prefix_length = strlen(mnt.name);
          st                    = static_cast<simgrid::surf::Storage*>(mnt.storage);
        }
        free(file_mount_name);
      }
      if (longest_prefix_length > 0) { /* Mount point found, split fullpath into mount_name and path+filename*/
        path       = (char*)xbt_malloc((strlen(fullpath) - longest_prefix_length + 1));
        mount_name = (char*)xbt_malloc((longest_prefix_length + 1));
        strncpy(mount_name, fullpath, longest_prefix_length + 1);
        strncpy(path, fullpath + longest_prefix_length, strlen(fullpath) - longest_prefix_length + 1);
        path[strlen(fullpath) - longest_prefix_length] = '\0';
        mount_name[longest_prefix_length]              = '\0';
      } else
        xbt_die("Can't find mount point for '%s' on '%s'", fullpath, piface_->name().c_str());

      XBT_DEBUG("OPEN %s on disk '%s'", path, st->getName());
      Action* action = st->open((const char*)mount_name, (const char*)path);
      free((char*)path);
      free((char*)mount_name);
      return action;
    }

    Action* HostImpl::close(surf_file_t fd)
    {
      simgrid::surf::Storage* st = findStorageOnMountList(fd->mount);
      XBT_DEBUG("CLOSE %s on disk '%s'", fd->name, st->getName());
      return st->close(fd);
    }

    Action* HostImpl::read(surf_file_t fd, sg_size_t size)
    {
      simgrid::surf::Storage* st = findStorageOnMountList(fd->mount);
      XBT_DEBUG("READ %s on disk '%s'", fd->name, st->getName());
      return st->read(fd, size);
    }

    Action* HostImpl::write(surf_file_t fd, sg_size_t size)
    {
      simgrid::surf::Storage* st = findStorageOnMountList(fd->mount);
      XBT_DEBUG("WRITE %s on disk '%s'", fd->name, st->getName());
      return st->write(fd, size);
    }

    int HostImpl::unlink(surf_file_t fd)
    {
      if (!fd) {
        XBT_WARN("No such file descriptor. Impossible to unlink");
        return -1;
      } else {

        simgrid::surf::Storage* st = findStorageOnMountList(fd->mount);
        /* Check if the file is on this storage */
        if (!xbt_dict_get_or_null(st->content_, fd->name)) {
          XBT_WARN("File %s is not on disk %s. Impossible to unlink", fd->name, st->getName());
          return -1;
        } else {
          XBT_DEBUG("UNLINK %s on disk '%s'", fd->name, st->getName());
          st->usedSize_ -= fd->size;

          // Remove the file from storage
          xbt_dict_remove(st->content_, fd->name);

          xbt_free(fd->name);
          xbt_free(fd->mount);
          xbt_free(fd);
          return 0;
        }
      }
    }

    sg_size_t HostImpl::getSize(surf_file_t fd)
    {
      return fd->size;
    }

    xbt_dynar_t HostImpl::getInfo(surf_file_t fd)
    {
      simgrid::surf::Storage* st = findStorageOnMountList(fd->mount);
      sg_size_t* psize           = xbt_new(sg_size_t, 1);
      *psize                     = fd->size;
      xbt_dynar_t info           = xbt_dynar_new(sizeof(void*), nullptr);
      xbt_dynar_push_as(info, sg_size_t*, psize);
      xbt_dynar_push_as(info, void*, fd->mount);
      xbt_dynar_push_as(info, void*, (void*)st->getName());
      xbt_dynar_push_as(info, void*, st->typeId_);
      xbt_dynar_push_as(info, void*, st->contentType_);

      return info;
    }

    sg_size_t HostImpl::fileTell(surf_file_t fd)
    {
      return fd->current_position;
    }

    int HostImpl::fileSeek(surf_file_t fd, sg_offset_t offset, int origin)
    {

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

    int HostImpl::fileMove(surf_file_t fd, const char* fullpath)
    {
      /* Check if the new full path is on the same mount point */
      if (!strncmp((const char*)fd->mount, fullpath, strlen(fd->mount))) {
        sg_size_t *psize, *new_psize;
        psize      = (sg_size_t*)xbt_dict_get_or_null(findStorageOnMountList(fd->mount)->content_, fd->name);
        new_psize  = xbt_new(sg_size_t, 1);
        *new_psize = *psize;
        if (psize) { // src file exists
          xbt_dict_remove(findStorageOnMountList(fd->mount)->content_, fd->name);
          char* path = (char*)xbt_malloc((strlen(fullpath) - strlen(fd->mount) + 1));
          strncpy(path, fullpath + strlen(fd->mount), strlen(fullpath) - strlen(fd->mount) + 1);
          xbt_dict_set(findStorageOnMountList(fd->mount)->content_, path, new_psize, nullptr);
          XBT_DEBUG("Move file from %s to %s, size '%llu'", fd->name, fullpath, *psize);
          free(path);
          return 0;
        } else {
          XBT_WARN("File %s doesn't exist", fd->name);
          return -1;
        }
      } else {
        XBT_WARN("New full path %s is not on the same mount point: %s. Action has been canceled.", fullpath, fd->mount);
        return -1;
      }
    }

    xbt_dynar_t HostImpl::getVms()
    {
      xbt_dynar_t dyn = xbt_dynar_new(sizeof(simgrid::surf::VirtualMachineImpl*), nullptr);

      for (VirtualMachineImpl* ws_vm : VirtualMachineImpl::allVms_) {
        if (this == ws_vm->getPm()->pimpl_)
          xbt_dynar_push(dyn, &ws_vm);
      }

      return dyn;
    }
    }
    }
