/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "Utils.hpp"
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <map>
#include <random>
#include <simgrid/kernel/resource/NetworkModelIntf.hpp>
#include <simgrid/s4u.hpp>
#include <smpi/smpi.h>
namespace sg4 = simgrid::s4u;

class NormalMixture : public Sampler {
  std::vector<std::normal_distribution<double>> mixture_;
  std::vector<double> prob_;
  std::mt19937& gen_;

public:
  NormalMixture(std::mt19937& gen) : gen_(gen) {}
  void append(double mean, double stddev, double prob)
  {
    mixture_.push_back(std::normal_distribution<double>(mean, stddev));
    prob_.push_back(prob);
  }
  double sample()
  {
    std::discrete_distribution<> d(prob_.begin(), prob_.end());
    int index    = d(gen_);
    auto& normal = mixture_[index];
    double value = normal(gen_);
    return value;
  }
};

/**
 * @brief Callback to set latency factor for a communication
 *
 * @param latency_base The base latency for this calibration (user-defined)
 * @param seg Segmentation (user-defined)
 * @param size Message size (simgrid)
 */
static double latency_factor_cb(double latency_base, const SegmentedRegression& seg, double size,
                                const sg4::Host* /*src*/, const sg4::Host* /*dst*/,
                                const std::vector<sg4::Link*>& /*links*/,
                                const std::unordered_set<sg4::NetZone*>& /*netzones*/)
{
  if (size < 63305)
    return 0.0; // no calibration for small messages

  return seg.sample(size) / latency_base;
}

/**
 * @brief Callback to set bandwidth factor for a communication
 *
 * @param bw_base The base bandwidth for this calibration (user-defined)
 * @param seg Segmentation (user-defined)
 * @param size Message size (simgrid)
 */
static double bw_factor_cb(double bw_base, const SegmentedRegression& seg, double size, const sg4::Host* /*src*/,
                           const sg4::Host* /*dst*/, const std::vector<sg4::Link*>& /*links*/,
                           const std::unordered_set<sg4::NetZone*>& /*netzones*/)
{
  if (size < 63305)
    return 1.0; // no calibration for small messages
  double est_bw = 1.0 / seg.get_coef(size);
  return est_bw / bw_base;
}

static double smpi_cost_cb(const SegmentedRegression& seg, double size, sg4::Host* /*src*/, sg4::Host* /*dst*/)
{
  return seg.sample(size);
}

static SegmentedRegression read_json_file(const std::string& jsonFile, std::mt19937& gen, bool read_coef = true)
{
  boost::property_tree::ptree pt;
  boost::property_tree::read_json(jsonFile, pt);
  std::unordered_map<double, std::shared_ptr<NormalMixture>> mixtures;
  std::unordered_map<double, double> coefs;
  printf("Starting parsing file: %s\n", jsonFile.c_str());
  pt = pt.get_child("seg"); // go to segments part
  SegmentedRegression seg(read_coef);
  for (const auto& it : pt) {
    double max    = it.second.get_child("max_x").get_value<double>();
    coefs[max]    = it.second.get_child("coefficient").get_value<double>();
    auto& mixture = mixtures[max];
    if (!mixture)
      mixture = std::make_shared<NormalMixture>(gen);
    mixture->append(it.second.get_child("mean").get_value<double>(), it.second.get_child("sd").get_value<double>(),
                    it.second.get_child("prob").get_value<double>());
  }
  for (const auto& i : mixtures) {
    seg.append(i.first, coefs[i.first], i.second);
  }
  return seg;
}

/** @brief Programmatic version of dahu */
extern "C" void load_platform(const sg4::Engine& e);
void load_platform(const sg4::Engine& e)
{
  // setting threshold sync/async modes
  e.set_config("smpi/async-small-thresh", 63305);
  e.set_config("smpi/send-is-detached-thresh", 63305);

  /* reading bandwidth and latency base value. It is the same for all regressions, get from first file */
  boost::property_tree::ptree pt;
  boost::property_tree::read_json("pingpong_ckmeans.json", pt);
  double bw_base  = pt.get_child("bandwidth_base").get_value<double>();
  double lat_base = pt.get_child("latency_base").get_value<double>();
  printf("Read bandwidth_base: %e latency_base: %e\n", bw_base, lat_base);

  load_dahu_platform(e, bw_base, lat_base);

  static std::mt19937 gen(42); // remove it from stack, since we need it after this this load_platform function is over

  /* setting network factors callbacks */
  simgrid::kernel::resource::NetworkModelIntf* model = e.get_netzone_root()->get_network_model();

  SegmentedRegression seg = read_json_file("pingpong_ckmeans.json", gen, false);
  model->set_lat_factor_cb(std::bind(&latency_factor_cb, lat_base, seg, std::placeholders::_1, std::placeholders::_2,
                                     std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

  model->set_bw_factor_cb(std::bind(&bw_factor_cb, bw_base, seg, std::placeholders::_1, std::placeholders::_2,
                                    std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

  seg = read_json_file("send_ckmeans.json", gen);
  smpi_register_op_cost_callback(SmpiOperation::SEND, std::bind(&smpi_cost_cb, seg, std::placeholders::_1,
                                                                std::placeholders::_2, std::placeholders::_3));

  seg = read_json_file("isend_ckmeans.json", gen);
  smpi_register_op_cost_callback(SmpiOperation::ISEND, std::bind(&smpi_cost_cb, seg, std::placeholders::_1,
                                                                 std::placeholders::_2, std::placeholders::_3));
  seg = read_json_file("recv_ckmeans.json", gen);
  smpi_register_op_cost_callback(SmpiOperation::RECV, std::bind(&smpi_cost_cb, seg, std::placeholders::_1,
                                                                std::placeholders::_2, std::placeholders::_3));
}