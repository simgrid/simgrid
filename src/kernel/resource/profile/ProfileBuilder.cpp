/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/ProfileBuilder.hpp"
#include "simgrid/forward.h"
#include "src/kernel/resource/profile/Profile.hpp"
#include "src/kernel/resource/profile/StochasticDatedValue.hpp"
#include "src/surf/surf_interface.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/intrusive/options.hpp>
#include <cstddef>
#include <fstream>
#include <sstream>


namespace simgrid {
namespace kernel {
namespace profile {

bool DatedValue::operator==(DatedValue const& e2) const
{
  return (fabs(date_ - e2.date_) < 0.0001) && (fabs(value_ - e2.value_) < 0.0001);
}

std::ostream& operator << (std::ostream& out, const DatedValue& dv) {
  out<<"(+"<<dv.date_<<','<<dv.value_<<')';
  return out;
}

/** @brief This callback implements the support for legacy profile syntax
*
* These profiles support two orthogonal concepts:
* - One-shot/Loop: the given list of {date,value} pairs can be repeated forever or just once. 
* - Deterministic/Stochastic: dates and/or values can be drawn according to a stochastic law.  
*
* A legacy profile is composed of "global" instructions and "local" instructions.
*
* Global instructions set properties that are true for all {date,value} pairs. They include
* - STOCHASTIC -> declare that the profile will use stochastic {date,value} pairs and not loop.
* - STOCHASTIC_LOOP -> declare that the profile will use stochastic {date,value} pairs and loop.
* - PERIODICITY -> declare the period for each iteration of the profile pattern.
* - LOOP_AFTER -> declare that the profile pattern will start over after a fixed delay
*
* Note that PERIODICITY and LOOP_AFTER have slightly different meanings. 
* PERIODICITY represents the delay between two iterations, for instance the time for the first pair of the pattern in two successive iterations.
* Hence, PERIODICITY only has meaning for completely deterministic profiles.
* LOOP_AFTER represents an additional delay between the last pair of a profile pattern iteration and the first pair of the next iteration.
*  
* Local instructions define one {date,value} pair, or more exactly one pattern. 
* In effect, when a profile loops or has stochastic components, the actual {date,value} pairs will be generated dynamically.    
* Roughly, a local instruction is a line with the following syntax:
* <date spec> <value spec>
* 
* Both date and value specifications may have one of the following formats:
* - <num> : in completely deterministic profiles, use this number
* - DET <num> : in stochastic profiles, use this number
* - NORM <mean> <standard deviation> : draw number from a normal distribution of given parameters   
* - UNIF <min> <max> : draw number from an uniform distribution of given parameters   
* - EXP <lambda> : draw number from an exponential distribution of given parameters   
*
* Note that in stochastic profiles, the date component is (necessarily) representing the delay between two pairs; 
* whereas in deterministic profiles, the date components represent the absolute date at which the elements are used in the first iteration.  
*/

class LegacyUpdateCb {

std::vector<StochasticDatedValue> pattern;

bool stochastic;

bool loop;

double loop_delay;

static bool is_comment_or_empty_line(const std::string& val)
{
  return (val[0] == '#' || val[0] == '\0' || val[0] == '%');
}

static bool is_normal_distribution(const std::string& val)
{
  return (val == "NORM" || val == "NORMAL" || val == "GAUSS" || val == "GAUSSIAN");
}

static bool is_exponential_distribution(const std::string& val)
{
  return (val == "EXP" || val == "EXPONENTIAL");
}

static bool is_uniform_distribution(const std::string& val)
{
  return (val == "UNIF" || val == "UNIFORM");
}

public: 
  LegacyUpdateCb(const std::string& input, double periodicity): stochastic(false),loop(periodicity>0),loop_delay(0)
  {
    int linecount                                    = 0;
    std::vector<std::string> list;
    double last_date=0;
    boost::split(list, input, boost::is_any_of("\n\r"));
    for (auto val : list) {
      simgrid::kernel::profile::StochasticDatedValue stochevent;
      linecount++;
      boost::trim(val);
      if (is_comment_or_empty_line(val))
        continue;
      if (sscanf(val.c_str(), "PERIODICITY %lg\n", &periodicity) == 1) {
        loop=true;
        continue;
      }
      if (sscanf(val.c_str(), "LOOPAFTER %lg\n", &loop_delay) == 1) {
        loop=true;
        continue;
      }
      if (val == "STOCHASTIC LOOP") {
        stochastic=true;
        loop=true;
        continue;
      }
      if (val == "STOCHASTIC") {
        stochastic=true;
        continue;
      }

      unsigned int i;
      unsigned int j;
      std::istringstream iss(val);
      std::vector<std::string> splittedval((std::istream_iterator<std::string>(iss)),
                                            std::istream_iterator<std::string>());

      xbt_assert(not splittedval.empty(), "Invalid profile line");

      if (splittedval[0] == "DET") {
        stochevent.date_law = Distribution::DET;
        i                   = 2;
      } else if (is_normal_distribution(splittedval[0])) {
        stochevent.date_law = Distribution::NORM;
        i                   = 3;
      } else if (is_exponential_distribution(splittedval[0])) {
        stochevent.date_law = Distribution::EXP;
        i                   = 2;
      } else if (is_uniform_distribution(splittedval[0])) {
        stochevent.date_law = Distribution::UNIF;
        i                   = 3;
      } else {
        xbt_assert(not stochastic);
        stochevent.date_law = Distribution::DET;
        i                   = 1;
      }

      xbt_assert(splittedval.size() > i, "Invalid profile line");
      if (i == 1 or i == 2) {
        stochevent.date_params = {std::stod(splittedval[i-1])};
      } else if (i == 3) {
        stochevent.date_params = {std::stod(splittedval[1]), std::stod(splittedval[2])};
      }

      if (splittedval[i] == "DET") {
        stochevent.value_law = Distribution::DET;
        j                    = 1;
      } else if (is_normal_distribution(splittedval[i])) {
        stochevent.value_law = Distribution::NORM;
        j                    = 2;
      } else if (is_exponential_distribution(splittedval[i])) {
        stochevent.value_law = Distribution::EXP;
        j                    = 1;
      } else if (is_uniform_distribution(splittedval[i])) {
        stochevent.value_law = Distribution::UNIF;
        j                    = 2;
      } else {
        xbt_assert(not stochastic);
        stochevent.value_law = Distribution::DET;
        j                   = 0;
      }

      xbt_assert(splittedval.size() > i + j, "Invalid profile line");
      if (j == 0 or j == 1) {
        stochevent.value_params = {std::stod(splittedval[i + j])};
      } else if (j == 2) {
        stochevent.value_params = {std::stod(splittedval[i + 1]), std::stod(splittedval[i + 2])};
      }

      if(not stochastic) {
        //In this mode, dates read from the string are absolute values
        double new_date=stochevent.date_params[0];
        xbt_assert(new_date>=0,"Profile time value is negative, why?");
        xbt_assert( last_date <= new_date ,
                    "%d: Invalid trace: Events must be sorted, but time %g > time %g.\n%s", linecount,
                    last_date, new_date, input.c_str());
        stochevent.date_params[0]-=last_date;
        last_date=new_date;
      }

      pattern.emplace_back(stochevent);
    }

    xbt_assert(not stochastic or periodicity <= 0, "If you want periodicity with stochastic profiles, use LOOP_AFTER");
    if(periodicity>0) {
      xbt_assert(loop and loop_delay==0);
      loop_delay=periodicity - last_date;
    }

    xbt_assert(loop_delay>=0,"Profile loop conditions are not realizable!");

  }

  double get_repeat_delay() {
    if(not stochastic and loop)
      return loop_delay;
    return -1.0;
  }

  void operator() (std::vector<DatedValue>& event_list) {
    size_t initial_size=event_list.size();
    if( loop or not initial_size ){
      for(auto dv : pattern)
        event_list.emplace_back(dv.get_datedvalue());
      if(initial_size)
        event_list.at(initial_size).date_+=loop_delay;
    }
  }


};


Profile* ProfileBuilder::from_string(const std::string& name, const std::string& input, double periodicity)
{
  LegacyUpdateCb cb(input, periodicity);
  return new Profile(name,cb,cb.get_repeat_delay());
}

Profile* ProfileBuilder::from_file(const std::string& path)
{
  xbt_assert(not path.empty(), "Cannot parse a trace from an empty filename");
  auto f = std::unique_ptr<std::ifstream>(surf_ifsopen(path));
  xbt_assert(not f->fail(), "Cannot open file '%s' (path=%s)", path.c_str(), (boost::join(surf_path, ":")).c_str());

  std::stringstream buffer;
  buffer << f->rdbuf();

  LegacyUpdateCb cb(buffer.str(), -1);
  return new Profile(path,cb,cb.get_repeat_delay());
}


Profile* ProfileBuilder::from_void() {
  static Profile void_profile("__void__",[](std::vector<DatedValue>&){},-1.0);
  return &void_profile;
}

Profile* ProfileBuilder::from_callback(const std::string& name, const std::function<UpdateCb>& cb, double repeat_delay) {
  return new Profile(name, cb, repeat_delay);
}


} // namespace profile
} // namespace kernel
} // namespace simgrid