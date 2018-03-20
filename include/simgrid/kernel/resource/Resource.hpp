/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

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
public:
  /**
   * @brief Constructor of LMM Resources
   *
   * @param model Model associated to this Resource
   * @param name The name of the Resource
   * @param constraint The lmm constraint associated to this Resource if it is part of a LMM component
   */
  Resource(Model* model, const std::string& name, lmm::Constraint* constraint);

  virtual ~Resource();

  /** @brief Get the Model of the current Resource */
  Model* model() const;

  /** @brief Get the name of the current Resource */
  const std::string& getName() const;
  /** @brief Get the name of the current Resource */
  const char* getCname() const;

  bool operator==(const Resource& other) const;

  /**
   * @brief Apply an event of external load event to that resource
   *
   * @param event What happened
   * @param value [TODO]
   */
  virtual void apply_event(TraceEvent* event, double value) = 0;

  /** @brief Check if the current Resource is used (if it currently serves an action) */
  virtual bool isUsed() = 0;

  /** @brief returns the current load (in flops per second, byte per second or similar) */
  virtual double getLoad();

  /** @brief Check if the current Resource is active */
  virtual bool isOn() const;
  /** @brief Check if the current Resource is shut down */
  virtual bool isOff() const;
  /** @brief Turn on the current Resource */
  virtual void turnOn();
  /** @brief Turn off the current Resource */
  virtual void turnOff();

private:
  std::string name_;
  Model* model_;
  bool isOn_ = true;

public: /* LMM */
  /** @brief Get the lmm constraint associated to this Resource if it is part of a LMM component (or null if none) */
  kernel::lmm::Constraint* constraint() const;

private:
  kernel::lmm::Constraint* const constraint_ = nullptr;

protected:
  struct Metric {
    double peak;       /**< The peak of the metric, ie its max value */
    double scale;      /**< Current availability of the metric according to the traces, in [0,1] */
    TraceEvent* event; /**< The associated trace event associated to the metric */
  };
};
} // namespace resource
} // namespace kernel
} // namespace simgrid

namespace std {
template <> class hash<simgrid::kernel::resource::Resource> {
public:
  std::size_t operator()(const simgrid::kernel::resource::Resource& r) const
  {
    return (std::size_t)xbt_str_hash(r.getCname());
  }
};
} // namespace std

#endif
