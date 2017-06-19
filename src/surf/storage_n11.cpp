/* Copyright (c) 2013-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "storage_n11.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "src/kernel/routing/NetPoint.hpp"
#include <math.h> /*ceil*/

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_storage);

/*************
 * CallBacks *
 *************/
extern std::map<std::string, storage_type_t> storage_types;

static void check_disk_attachment()
{
  for (auto s : *simgrid::surf::StorageImpl::storagesMap()) {
    simgrid::kernel::routing::NetPoint* host_elm = sg_netpoint_by_name_or_null(s.second->attach_.c_str());
    if (not host_elm)
      surf_parse_error("Unable to attach storage %s: host %s does not exist.", s.second->cname(),
                       s.second->attach_.c_str());
    else
      s.second->piface_.attached_to_ = sg_host_by_name(s.second->attach_.c_str());
  }
}

void storage_register_callbacks()
{
  simgrid::s4u::onPlatformCreated.connect(check_disk_attachment);
  instr_routing_define_callbacks();
}

/*********
 * Model *
 *********/

void surf_storage_model_init_default()
{
  surf_storage_model = new simgrid::surf::StorageN11Model();
  all_existing_models->push_back(surf_storage_model);
}

namespace simgrid {
namespace surf {

StorageImpl* StorageN11Model::createStorage(const char* id, const char* type_id, const char* content_name,
                                            const char* attach)
{
  storage_type_t storage_type = storage_types.at(type_id);

  double Bread =
      surf_parse_get_bandwidth(storage_type->model_properties->at("Bread").c_str(), "property Bread, storage", type_id);
  double Bwrite = surf_parse_get_bandwidth(storage_type->model_properties->at("Bwrite").c_str(),
                                           "property Bwrite, storage", type_id);

  StorageImpl* storage = new StorageN11(this, id, maxminSystem_, Bread, Bwrite, type_id, (char*)content_name,
                                        storage_type->size, (char*)attach);
  storageCreatedCallbacks(storage);

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s'\n\t\tBread '%f'\n", id, type_id, Bread);

  p_storageList.push_back(storage);

  return storage;
}

double StorageN11Model::nextOccuringEvent(double now)
{
  double min_completion = StorageModel::nextOccuringEventFull(now);

  for(auto storage: p_storageList) {
    double rate = 0;
    // Foreach write action on that disk
    for (auto write_action: storage->writeActions_) {
      rate += lmm_variable_getvalue(write_action->getVariable());
    }
    if(rate > 0)
      min_completion = MIN(min_completion, (storage->size_-storage->usedSize_)/rate);
  }

  return min_completion;
}

void StorageN11Model::updateActionsState(double /*now*/, double delta)
{

  ActionList *actionSet = getRunningActionSet();
  for(ActionList::iterator it(actionSet->begin()), itNext=it, itend(actionSet->end())
      ; it != itend ; it=itNext) {
    ++itNext;

    StorageAction *action = static_cast<StorageAction*>(&*it);

    if (action->type_ == WRITE) {
      // Update the disk usage
      // Update the file size
      // For each action of type write
      double current_progress = delta * lmm_variable_getvalue(action->getVariable());
      long int incr = current_progress;

      XBT_DEBUG("%s:\n\t progress =  %.2f, current_progress = %.2f, incr = %ld, lrint(1) = %ld, lrint(2) = %ld",
                action->file_->name, action->progress_, current_progress, incr,
                lrint(action->progress_ + current_progress), lrint(action->progress_) + incr);

      /* take care of rounding error accumulation */
      if (lrint(action->progress_ + current_progress) > lrint(action->progress_) + incr)
        incr++;

      action->progress_ += current_progress;

      action->storage_->usedSize_ += incr;     // disk usage
      action->file_->current_position += incr; // current_position
      //  which becomes the new file size
      action->file_->size = action->file_->current_position;

      action->storage_->content_->erase(action->file_->name);
      action->storage_->content_->insert({action->file_->name, action->file_->size});
    }

    action->updateRemains(lmm_variable_getvalue(action->getVariable()) * delta);

    if (action->getMaxDuration() > NO_MAX_DURATION)
      action->updateMaxDuration(delta);

    if (action->getRemainsNoUpdate() > 0 && lmm_get_variable_weight(action->getVariable()) > 0 &&
        action->storage_->usedSize_ == action->storage_->size_) {
      action->finish();
      action->setState(Action::State::failed);
    } else if (((action->getRemainsNoUpdate() <= 0) && (lmm_get_variable_weight(action->getVariable()) > 0)) ||
               ((action->getMaxDuration() > NO_MAX_DURATION) && (action->getMaxDuration() <= 0))) {
      action->finish();
      action->setState(Action::State::done);
    }
  }
}

/************
 * Resource *
 ************/

StorageN11::StorageN11(StorageModel* model, const char* name, lmm_system_t maxminSystem, double bread, double bwrite,
                       const char* type_id, char* content_name, sg_size_t size, char* attach)
    : StorageImpl(model, name, maxminSystem, bread, bwrite, type_id, content_name, size, attach)
{
  XBT_DEBUG("Create resource with Bread '%f' Bwrite '%f' and Size '%llu'", bread, bwrite, size);
  simgrid::s4u::Storage::onCreation(this->piface_);
}

StorageAction *StorageN11::open(const char* mount, const char* path)
{
  XBT_DEBUG("\tOpen file '%s'",path);

  sg_size_t size;
  // if file does not exist create an empty file
  if (content_->find(path) != content_->end())
    size = content_->at(path);
  else {
    size = 0;
    content_->insert({path, size});
    XBT_DEBUG("File '%s' was not found, file created.",path);
  }
  surf_file_t file = xbt_new0(s_surf_file_t,1);
  file->name = xbt_strdup(path);
  file->size = size;
  file->mount = xbt_strdup(mount);
  file->current_position = 0;

  StorageAction* action = new StorageN11Action(model(), 0, isOff(), this, OPEN);
  action->file_         = file;

  return action;
}

StorageAction *StorageN11::close(surf_file_t fd)
{
  XBT_DEBUG("\tClose file '%s' size '%llu'", fd->name, fd->size);
  // unref write actions from storage
  for (std::vector<StorageAction*>::iterator it = writeActions_.begin(); it != writeActions_.end();) {
    StorageAction *write_action = *it;
    if ((write_action->file_) == fd) {
      write_action->unref();
      it = writeActions_.erase(it);
    } else {
      ++it;
    }
  }
  free(fd->name);
  free(fd->mount);
  xbt_free(fd);
  StorageAction* action = new StorageN11Action(model(), 0, isOff(), this, CLOSE);
  return action;
}

StorageAction *StorageN11::read(surf_file_t fd, sg_size_t size)
{
  if(fd->current_position + size > fd->size){
    if (fd->current_position > fd->size){
      size = 0;
    } else {
      size = fd->size - fd->current_position;
    }
    fd->current_position = fd->size;
  }
  else
    fd->current_position += size;

  StorageAction* action = new StorageN11Action(model(), size, isOff(), this, READ);
  return action;
}

StorageAction *StorageN11::write(surf_file_t fd, sg_size_t size)
{
  char *filename = fd->name;
  XBT_DEBUG("\tWrite file '%s' size '%llu/%llu'",filename,size,fd->size);

  StorageAction* action = new StorageN11Action(model(), size, isOff(), this, WRITE);
  action->file_         = fd;
  /* Substract the part of the file that might disappear from the used sized on the storage element */
  usedSize_ -= (fd->size - fd->current_position);
  // If the storage is full before even starting to write
  if(usedSize_==size_) {
    action->setState(Action::State::failed);
  }
  return action;
}

/**********
 * Action *
 **********/

StorageN11Action::StorageN11Action(Model* model, double cost, bool failed, StorageImpl* storage,
                                   e_surf_action_storage_type_t type)
    : StorageAction(model, cost, failed, lmm_variable_new(model->getMaxminSystem(), this, 1.0, -1.0, 3), storage, type)
{
  XBT_IN("(%s,%g", storage->cname(), cost);

  // Must be less than the max bandwidth for all actions
  lmm_expand(model->getMaxminSystem(), storage->constraint(), getVariable(), 1.0);
  switch(type) {
  case OPEN:
  case CLOSE:
  case STAT:
    break;
  case READ:
    lmm_expand(model->getMaxminSystem(), storage->constraintRead_, getVariable(), 1.0);
    break;
  case WRITE:
    lmm_expand(model->getMaxminSystem(), storage->constraintWrite_, getVariable(), 1.0);

    //TODO there is something annoying with what's below. Have to sort it out...
    //    Action *action = this;
    //    storage->p_writeActions->push_back(action);
    //    ref();
    break;
  default:
    THROW_UNIMPLEMENTED;
  }
  XBT_OUT();
}

int StorageN11Action::unref()
{
  refcount_--;
  if (not refcount_) {
    if (action_hook.is_linked())
      stateSet_->erase(stateSet_->iterator_to(*this));
    if (getVariable())
      lmm_variable_free(getModel()->getMaxminSystem(), getVariable());
    xbt_free(getCategory());
    delete this;
    return 1;
  }
  return 0;
}

void StorageN11Action::cancel()
{
  setState(Action::State::failed);
}

void StorageN11Action::suspend()
{
  XBT_IN("(%p)", this);
  if (suspended_ != 2) {
    lmm_update_variable_weight(getModel()->getMaxminSystem(), getVariable(), 0.0);
    suspended_ = 1;
  }
  XBT_OUT();
}

void StorageN11Action::resume()
{
  THROW_UNIMPLEMENTED;
}

bool StorageN11Action::isSuspended()
{
  return suspended_ == 1;
}

void StorageN11Action::setMaxDuration(double /*duration*/)
{
  THROW_UNIMPLEMENTED;
}

void StorageN11Action::setPriority(double /*priority*/)
{
  THROW_UNIMPLEMENTED;
}

}
}
