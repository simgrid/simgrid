/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <map>
#include <memory>
#include <simgrid/s4u.hpp>
#include <vector>

/** @brief Creates a platform based on dahu cluster in Grid'5000 */
void load_dahu_platform(const simgrid::s4u::Engine& e, double bw, double lat);

/** @brief Noise generation interface */
class Sampler {
public:
  virtual double sample() = 0;
};

/** @brief Segmented noise generation. Use different sampler for each message size */
class SegmentedRegression {
  std::map<double, std::shared_ptr<Sampler>> segments_;
  std::map<double, double> coef_;
  bool use_coef_;
  std::shared_ptr<Sampler> get_sampler(double x) const
  {
    auto sampler = segments_.upper_bound(x);
    if (sampler->first >= x)
      return sampler->second;
    THROW_IMPOSSIBLE;
  }

public:
  explicit SegmentedRegression(bool use = true) : use_coef_(use) {}
  void append(double max, double coef, std::shared_ptr<Sampler> sampler)
  {
    coef_[max]     = coef;
    segments_[max] = std::move(sampler);
  }

  double sample(double x) const
  {
    double value = get_sampler(x)->sample();
    if (use_coef_)
      value += x * coef_.upper_bound(x)->second;
    return value;
  }

  double get_coef(double x) const { return coef_.upper_bound(x)->second; }
};