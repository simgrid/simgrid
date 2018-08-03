/* Copyright (c) 2013-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "storage_n11.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/xml/platf.hpp"
#include "surf/surf.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_storage);

/*************
 * CallBacks *
 *************/
extern std::map<std::string, simgrid::surf::StorageType*> storage_types;

void check_disk_attachment()
{
  for (auto const& s : simgrid::s4u::Engine::get_instance()->get_all_storages()) {
    simgrid::kernel::routing::NetPoint* host_elm = sg_netpoint_by_name_or_null(s->get_impl()->getHost().c_str());
    if (not host_elm)
      surf_parse_error(std::string("Unable to attach storage ") + s->get_cname() + ": host " +
                       s->get_impl()->getHost().c_str() + " does not exist.");
    else
      s->set_host(sg_host_by_name(s->get_impl()->getHost().c_str()));
  }
}

/*********
 * Model *
 *********/

void surf_storage_model_init_default()
{
  surf_storage_model = new simgrid::surf::StorageN11Model();
}

namespace simgrid {
namespace surf {

StorageN11Model::StorageN11Model()
{
  all_existing_models.push_back(this);
}

StorageImpl* StorageN11Model::createStorage(std::string id, std::string type_id, std::string content_name,
                                            std::string attach)
{
  StorageType* storage_type = storage_types.at(type_id);

  double Bread = surf_parse_get_bandwidth(storage_type->model_properties->at("Bread").c_str(),
                                          "property Bread, storage", type_id.c_str());
  double Bwrite = surf_parse_get_bandwidth(storage_type->model_properties->at("Bwrite").c_str(),
                                           "property Bwrite, storage", type_id.c_str());

  StorageImpl* storage =
      new StorageN11(this, id, get_maxmin_system(), Bread, Bwrite, type_id, content_name, storage_type->size, attach);

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s'\n\t\tBread '%f'\n", id.c_str(), type_id.c_str(),
            Bread);

  return storage;
}

double StorageN11Model::next_occuring_event(double now)
{
  return StorageModel::next_occuring_event_full(now);
}

void StorageN11Model::update_actions_state(double /*now*/, double delta)
{
  for (auto it = std::begin(*get_started_action_set()); it != std::end(*get_started_action_set());) {
    StorageAction& action = static_cast<StorageAction&>(*it);
    ++it; // increment iterator here since the following calls to action.finish() may invalidate it
    action.update_remains(lrint(action.get_variable()->get_value() * delta));

    if (action.get_max_duration() > NO_MAX_DURATION)
      action.update_max_duration(delta);

    if (((action.get_remains_no_update() <= 0) && (action.get_variable()->get_weight() > 0)) ||
        ((action.get_max_duration() > NO_MAX_DURATION) && (action.get_max_duration() <= 0))) {
      action.finish(kernel::resource::Action::State::FINISHED);
    }
  }
}

/************
 * Resource *
 ************/

StorageN11::StorageN11(StorageModel* model, std::string name, kernel::lmm::System* maxminSystem, double bread,
                       double bwrite, std::string type_id, std::string content_name, sg_size_t size, std::string attach)
    : StorageImpl(model, name, maxminSystem, bread, bwrite, type_id, content_name, size, attach)
{
  XBT_DEBUG("Create resource with Bread '%f' Bwrite '%f' and Size '%llu'", bread, bwrite, size);
  simgrid::s4u::Storage::on_creation(this->piface_);
}

StorageAction* StorageN11::io_start(sg_size_t size, s4u::Io::OpType type)
{
  return new StorageN11Action(get_model(), size, is_off(), this, type);
}

StorageAction* StorageN11::read(sg_size_t size)
{
  return new StorageN11Action(get_model(), size, is_off(), this, s4u::Io::OpType::READ);
}

StorageAction* StorageN11::write(sg_size_t size)
{
  return new StorageN11Action(get_model(), size, is_off(), this, s4u::Io::OpType::WRITE);
}

/**********
 * Action *
 **********/

StorageN11Action::StorageN11Action(kernel::resource::Model* model, double cost, bool failed, StorageImpl* storage,
                                   s4u::Io::OpType type)
    : StorageAction(model, cost, failed, model->get_maxmin_system()->variable_new(this, 1.0, -1.0, 3), storage, type)
{
  XBT_IN("(%s,%g", storage->get_cname(), cost);

  // Must be less than the max bandwidth for all actions
  model->get_maxmin_system()->expand(storage->get_constraint(), get_variable(), 1.0);
  switch(type) {
    case s4u::Io::OpType::READ:
      model->get_maxmin_system()->expand(storage->constraintRead_, get_variable(), 1.0);
      break;
    case s4u::Io::OpType::WRITE:
      model->get_maxmin_system()->expand(storage->constraintWrite_, get_variable(), 1.0);
      break;
    default:
      THROW_UNIMPLEMENTED;
  }
  XBT_OUT();
}

void StorageN11Action::cancel()
{
  set_state(Action::State::FAILED);
}

void StorageN11Action::suspend()
{
  XBT_IN("(%p)", this);
  if (suspended_ != Action::SuspendStates::sleeping) {
    get_model()->get_maxmin_system()->update_variable_weight(get_variable(), 0.0);
    suspended_ = Action::SuspendStates::suspended;
  }
  XBT_OUT();
}

void StorageN11Action::resume()
{
  THROW_UNIMPLEMENTED;
}

void StorageN11Action::set_max_duration(double /*duration*/)
{
  THROW_UNIMPLEMENTED;
}

void StorageN11Action::set_priority(double /*priority*/)
{
  THROW_UNIMPLEMENTED;
}
void StorageN11Action::update_remains_lazy(double /*now*/)
{
  THROW_IMPOSSIBLE;
}
}
}
