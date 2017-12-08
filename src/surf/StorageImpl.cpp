/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "StorageImpl.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "surf_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_storage, surf, "Logging specific to the SURF storage module");

simgrid::surf::StorageModel* surf_storage_model = nullptr;

namespace simgrid {
namespace surf {

/*************
 * Callbacks *
 *************/

simgrid::xbt::signal<void(StorageImpl*)> storageCreatedCallbacks;
simgrid::xbt::signal<void(StorageImpl*)> storageDestructedCallbacks;
simgrid::xbt::signal<void(StorageImpl*, int, int)> storageStateChangedCallbacks; // signature: wasOn, isOn
simgrid::xbt::signal<void(StorageAction*, Action::State, Action::State)> storageActionStateChangedCallbacks;

/* List of storages */
std::unordered_map<std::string, StorageImpl*>* StorageImpl::storages =
    new std::unordered_map<std::string, StorageImpl*>();

StorageImpl* StorageImpl::byName(std::string name)
{
  if (storages->find(name) == storages->end())
    return nullptr;
  return storages->at(name);
}

/*********
 * Model *
 *********/

StorageModel::StorageModel() : Model()
{
  maxminSystem_ = new simgrid::kernel::lmm::System(true /* lazy update */);
}

StorageModel::~StorageModel()
{
  surf_storage_model = nullptr;
}

/************
 * Resource *
 ************/

StorageImpl::StorageImpl(Model* model, std::string name, lmm_system_t maxminSystem, double bread, double bwrite,
                         std::string type_id, std::string content_name, sg_size_t size, std::string attach)
    : Resource(model, name.c_str(), maxminSystem->constraint_new(this, std::max(bread, bwrite)))
    , piface_(this)
    , typeId_(type_id)
    , content_name(content_name)
    , size_(size)
    , attach_(attach)
{
  StorageImpl::turnOn();
  XBT_DEBUG("Create resource with Bread '%f' Bwrite '%f' and Size '%llu'", bread, bwrite, size);
  constraintRead_  = maxminSystem->constraint_new(this, bread);
  constraintWrite_ = maxminSystem->constraint_new(this, bwrite);
  storages->insert({name, this});
}

StorageImpl::~StorageImpl()
{
  storageDestructedCallbacks(this);
}


bool StorageImpl::isUsed()
{
  THROW_UNIMPLEMENTED;
  return false;
}

void StorageImpl::apply_event(tmgr_trace_event_t /*event*/, double /*value*/)
{
  THROW_UNIMPLEMENTED;
}

void StorageImpl::turnOn()
{
  if (isOff()) {
    Resource::turnOn();
    storageStateChangedCallbacks(this, 0, 1);
  }
}
void StorageImpl::turnOff()
{
  if (isOn()) {
    Resource::turnOff();
    storageStateChangedCallbacks(this, 1, 0);
  }
}

/**********
 * Action *
 **********/
void StorageAction::setState(Action::State state)
{
  Action::State old = getState();
  Action::setState(state);
  storageActionStateChangedCallbacks(this, old, state);
}
}
}
