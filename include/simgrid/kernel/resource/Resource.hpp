/* Copyright (c) 2004-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_RESOURCE_RESOURCE_HPP
#define SIMGRID_KERNEL_RESOURCE_RESOURCE_HPP

#include <simgrid/forward.h>
#include <xbt/signal.hpp>
#include <xbt/str.h>
#include <xbt/utility.hpp>

#include <string>

namespace simgrid {
namespace kernel {
namespace resource {

/** @ingroup SURF_interface
 * @brief SURF resource interface class
 * @details This is the ancestor class of every resources in SimGrid, such as links, CPU or storage
 */
class XBT_PUBLIC Resource {
  std::string name_;
  Model* model_;
  bool is_on_ = true;

  lmm::Constraint* const constraint_;

protected:
  struct Metric {
    double peak;           /**< The peak of the metric, ie its max value */
    double scale;          /**< Current availability of the metric according to the profiles, in [0,1] */
    profile::Event* event; /**< The associated profile event associated to the metric */
  };

public:
  /**
   * @brief Constructor of LMM Resources
   *
   * @param model Model associated to this Resource
   * @param name The name of the Resource
   * @param constraint The lmm constraint associated to this Resource if it is part of a LMM component
   */
  Resource(Model* model, const std::string& name, lmm::Constraint* constraint)
      : name_(name), model_(model), constraint_(constraint)
  {
  }

  virtual ~Resource() = default;

  /** @brief Get the Model of the current Resource */
  Model* get_model() const { return model_; }

  /** @brief Get the name of the current Resource */
  const std::string& get_name() const { return name_; }
  /** @brief Get the name of the current Resource */
  const char* get_cname() const { return name_.c_str(); }

  bool operator==(const Resource& other) const { return name_ == other.name_; }

  /** @brief Apply an event of external load event to that resource */
  virtual void apply_event(profile::Event* event, double value) = 0;

  /** @brief Check if the current Resource is used (if it currently serves an action) */
  virtual bool is_used() = 0;

  /** @brief returns the current load due to activities (in flops per second, byte per second or similar)
   *
   * The load due to external usages modeled by profile files is ignored.*/
  virtual double get_load() const;

  /** @brief Check if the current Resource is active */
  virtual bool is_on() const { return is_on_; }
  /** @brief Turn on the current Resource */
  virtual void turn_on() { is_on_ = true; }
  /** @brief Turn off the current Resource */
  virtual void turn_off() { is_on_ = false; }
  /** @brief setup the profile file with states events (ON or OFF). The profile must contain boolean values. */
  virtual void set_state_profile(profile::Profile* profile);

  /** @brief Get the lmm constraint associated to this Resource if it is part of a LMM component (or null if none) */
  lmm::Constraint* get_constraint() const { return constraint_; }

  profile::Event* state_event_ = nullptr;
};
} // namespace resource
} // namespace kernel
} // namespace simgrid

namespace std {
template <> class hash<simgrid::kernel::resource::Resource> {
public:
  std::size_t operator()(const simgrid::kernel::resource::Resource& r) const
  {
    return (std::size_t)xbt_str_hash(r.get_cname());
  }
};
} // namespace std

#endif
