/* Copyright (c) 2013-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "storage_n11.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/kernel/routing/NetPoint.hpp"
#include "xbt/utility.hpp"
#include <cmath> /*ceil*/

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_storage);

/*************
 * CallBacks *
 *************/
extern std::map<std::string, simgrid::surf::StorageType*> storage_types;

static void check_disk_attachment()
{
  for (auto const& s : *simgrid::surf::StorageImpl::storagesMap()) {
    simgrid::kernel::routing::NetPoint* host_elm = sg_netpoint_by_name_or_null(s.second->getHost().c_str());
    if (not host_elm)
      surf_parse_error(std::string("Unable to attach storage ") + s.second->getCname() + ": host " +
                       s.second->getHost() + " does not exist.");
    else
      s.second->piface_.attached_to_ = sg_host_by_name(s.second->getHost().c_str());
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

StorageImpl* StorageN11Model::createStorage(std::string id, std::string type_id, std::string content_name,
                                            std::string attach)
{
  StorageType* storage_type = storage_types.at(type_id);

  double Bread = surf_parse_get_bandwidth(storage_type->model_properties->at("Bread").c_str(),
                                          "property Bread, storage", type_id.c_str());
  double Bwrite = surf_parse_get_bandwidth(storage_type->model_properties->at("Bwrite").c_str(),
                                           "property Bwrite, storage", type_id.c_str());

  StorageImpl* storage =
      new StorageN11(this, id, maxminSystem_, Bread, Bwrite, type_id, content_name, storage_type->size, attach);
  storageCreatedCallbacks(storage);

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s'\n\t\tBread '%f'\n", id.c_str(), type_id.c_str(),
            Bread);

  p_storageList.push_back(storage);

  return storage;
}

double StorageN11Model::nextOccuringEvent(double now)
{
  return StorageModel::nextOccuringEventFull(now);
}

void StorageN11Model::updateActionsState(double /*now*/, double delta)
{
  for (auto it = std::begin(*getRunningActionSet()); it != std::end(*getRunningActionSet());) {
    StorageAction& action = static_cast<StorageAction&>(*it);
    ++it; // increment iterator here since the following calls to action.finish() may invalidate it
    action.updateRemains(lrint(action.getVariable()->get_value() * delta));

    if (action.getMaxDuration() > NO_MAX_DURATION)
      action.updateMaxDuration(delta);

    if (((action.getRemainsNoUpdate() <= 0) && (action.getVariable()->get_weight() > 0)) ||
        ((action.getMaxDuration() > NO_MAX_DURATION) && (action.getMaxDuration() <= 0))) {
      action.finish(Action::State::done);
    }
  }
}

/************
 * Resource *
 ************/

StorageN11::StorageN11(StorageModel* model, std::string name, lmm_system_t maxminSystem, double bread, double bwrite,
                       std::string type_id, std::string content_name, sg_size_t size, std::string attach)
    : StorageImpl(model, name, maxminSystem, bread, bwrite, type_id, content_name, size, attach)
{
  XBT_DEBUG("Create resource with Bread '%f' Bwrite '%f' and Size '%llu'", bread, bwrite, size);
  simgrid::s4u::Storage::onCreation(this->piface_);
}

StorageAction* StorageN11::read(sg_size_t size)
{
  return new StorageN11Action(model(), size, isOff(), this, READ);
}

StorageAction* StorageN11::write(sg_size_t size)
{
  return new StorageN11Action(model(), size, isOff(), this, WRITE);
}

/**********
 * Action *
 **********/

StorageN11Action::StorageN11Action(Model* model, double cost, bool failed, StorageImpl* storage,
                                   e_surf_action_storage_type_t type)
    : StorageAction(model, cost, failed, model->getMaxminSystem()->variable_new(this, 1.0, -1.0, 3), storage, type)
{
  XBT_IN("(%s,%g", storage->getCname(), cost);

  // Must be less than the max bandwidth for all actions
  model->getMaxminSystem()->expand(storage->constraint(), getVariable(), 1.0);
  switch(type) {
  case READ:
    model->getMaxminSystem()->expand(storage->constraintRead_, getVariable(), 1.0);
    break;
  case WRITE:
    model->getMaxminSystem()->expand(storage->constraintWrite_, getVariable(), 1.0);
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
      simgrid::xbt::intrusive_erase(*stateSet_, *this);
    if (getVariable())
      getModel()->getMaxminSystem()->variable_free(getVariable());
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
    getModel()->getMaxminSystem()->update_variable_weight(getVariable(), 0.0);
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

void StorageN11Action::setSharingWeight(double /*priority*/)
{
  THROW_UNIMPLEMENTED;
}

}
}
