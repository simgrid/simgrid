/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example validates the behaviour in presence of node and link fault.
 * Each test scenario consists in one host/actor (named sender) sending one message to another host/actor.
 * The space to cover is quite large, since we consider:
 * * communication types (eager, rendez-vous, one-sided=detached) 
 * * use type (synchronous, asynchronous, init)
 * * fault type (sender node, link, receiver node)
 * * any legal permutation of the scenario steps
 *
 * This program also presents a way to simulate applications that are resilient to links and node faults. 
 * Essentially, it catches exceptions related to communications and it clears the mailboxes when one of the nodes gets turned off. 
 * However, this model would suppose that there would be 2 mailboxes for each pair of nodes, which is probably unacceptable.
 * 
 */

#include <algorithm>
#include <random>
#include <simgrid/kernel/ProfileBuilder.hpp>
#include <simgrid/s4u.hpp>
#include <sstream>
#include <time.h>
#include <vector>

namespace sg4 = simgrid::s4u;
namespace pr  = simgrid::kernel::profile;

XBT_LOG_NEW_DEFAULT_CATEGORY(comm_fault_scenarios, "Messages specific for this s4u example");

/*************************************************************************************************/

// Constants for platform configuration
constexpr double HostComputePower = 1e9;  // FLOPs
constexpr double LinkBandwidth    = 1e9;  // Bytes/second
constexpr double LinkLatency      = 1e-6; // Seconds

// Constants for application behaviour
constexpr uint64_t MsgSize = LinkBandwidth / 2;

/*************************************************************************************************/

enum class CommType {
  EAGER_SYNC,
  EAGER_ASYNC,
  EAGER_INIT,
  RDV_SYNC,
  RDV_ASYNC,
  RDV_INIT,
  ONESIDE_SYNC,
  ONESIDE_ASYNC
  //ONESIDE_INIT is equivalent to ONESIDE_ASYNC
};

enum class Action { SLEEP, PUT, GET, START, WAIT, DIE, END };

static const char* to_string(const Action x)
{
  switch (x) {
    case Action::END:
      return "Success";
    case Action::SLEEP:
      return "Sleep";
    case Action::PUT:
      return "Put";
    case Action::GET:
      return "Get";
    case Action::START:
      return "Start";
    case Action::WAIT:
      return "Wait";
    case Action::DIE:
      return "Die!";
  };
  return "";
}

struct Step {
  double rel_time; // Time relative to Scenario startTime
  enum { STATE, ACTION } type;
  enum { LNK, SND, RCV } entity;
  Action action_type;
  bool new_state;
};

struct Scenario {
  CommType type;
  double start_time;
  double duration;
  Action snd_expected;
  Action rcv_expected;
  std::vector<Step> steps;
  int index;
};

static std::string to_string(const Scenario& s)
{
  std::stringstream ss;
  ss <<"#"<< s.index << "[" << s.start_time << "s," << s.start_time + s.duration << "s[: (";
  switch (s.type) {
    case CommType::EAGER_SYNC:
      ss << "EAGER_SYNC";
      break;
    case CommType::EAGER_ASYNC:
      ss << "EAGER_ASYNC";
      break;
    case CommType::EAGER_INIT:
      ss << "EAGER_INIT";
      break;
    case CommType::RDV_SYNC:
      ss << "RDV_SYNC";
      break;
    case CommType::RDV_ASYNC:
      ss << "RDV_ASYNC";
      break;
    case CommType::RDV_INIT:
      ss << "RDV_INIT";
      break;
    case CommType::ONESIDE_SYNC:
      ss << "ONESIDE_SYNC";
      break;
    case CommType::ONESIDE_ASYNC:
      ss << "ONESIDE_ASYNC";
      break;
  }

  ss << ") Expected: S:" << to_string(s.snd_expected) << " R:" << to_string(s.rcv_expected) << " Steps: ";
  for (const Step& step : s.steps) {
    ss << "+" << step.rel_time << "s:";
    switch (step.entity) {
      case Step::LNK:
        ss << "LNK";
        break;
      case Step::SND:
        ss << "SND";
        break;
      case Step::RCV:
        ss << "RCV";
        break;
    }

    if (step.type == Step::STATE) {
      ss << "->";
      if (step.new_state)
        ss << "ON";
      else
        ss << "OFF";
    } else {
      ss << "." << to_string(step.action_type);
    }
    ss << " ";
  }
  return ss.str().c_str();
}

std::vector<Scenario> scenarios;
sg4::Mailbox* mbox_eager = nullptr;
sg4::Mailbox* mbox_rdv   = nullptr;

class SendAgent {

  static int run;
  static size_t scenario;
  int id;
  sg4::Host* other_host;

  sg4::CommPtr do_put(CommType type, double& send_value)
  {
    switch (type) {
      case CommType::EAGER_SYNC:
        mbox_eager->put(&send_value, MsgSize);
        return nullptr;
      case CommType::EAGER_ASYNC:
        return mbox_eager->put_async(&send_value, MsgSize);
      case CommType::EAGER_INIT:
        return mbox_eager->put_init(&send_value, MsgSize);
      case CommType::RDV_SYNC:
        mbox_rdv->put(&send_value, MsgSize);
        return nullptr;
      case CommType::RDV_ASYNC:
        return mbox_rdv->put_async(&send_value, MsgSize);
      case CommType::RDV_INIT:
        return mbox_rdv->put_init(&send_value, MsgSize);
      case CommType::ONESIDE_SYNC:
        sg4::Comm::sendto(sg4::this_actor::get_host(), other_host, MsgSize);
        return nullptr;
      case CommType::ONESIDE_ASYNC:
        return sg4::Comm::sendto_async(sg4::this_actor::get_host(), other_host, MsgSize);
    }
    return nullptr;
  }

  void send_message(const Scenario& s)
  {
    std::string scenario_string = to_string(s);
     XBT_DEBUG("Will try: %s", scenario_string.c_str());
    double send_value;
    sg4::CommPtr comm = nullptr;
    Action expected = s.snd_expected;
    double end_time = s.start_time + s.duration;
    send_value        = end_time;
    size_t step_index = 0;

    sg4::this_actor::sleep_until(s.start_time);

    //Make sure we have a clean slate
    xbt_assert(not mbox_eager->listen(),"Eager mailbox should be empty when starting a test");
    xbt_assert(not mbox_rdv->listen(),"RDV mailbox should be empty when starting a test");

    for (; step_index < s.steps.size(); step_index++) {
      const Step& step = s.steps[step_index];
      if (step.entity != Step::SND || step.type != Step::ACTION)
        continue;

      try {
        sg4::this_actor::sleep_until(s.start_time + step.rel_time);
      } catch (std::exception& e) {
        XBT_DEBUG("During Sleep, failed to send message because of a %s exception (%s)", typeid(e).name(), e.what());
        break;
      }

      // Check if the other host is still OK.
      if (not other_host->is_on())
        break;

      // Perform the action
      try {
        switch (step.action_type) {
          case Action::PUT:
            comm=do_put(s.type, send_value);
            break;
          case Action::START:
            comm->start();
            break;
          case Action::WAIT:
            comm->wait();
            break;
          default:
            xbt_die("Not a valid action for SND");
        }
      } catch (std::exception& e) {
        XBT_DEBUG("During %s, failed to send message because of a %s exception (%s)", to_string(step.action_type),
                  typeid(e).name(), e.what());
        break;
      }
    }

    try {
      sg4::this_actor::sleep_until(end_time);
    } catch (std::exception& e) {
      XBT_DEBUG("During Sleep, failed to send message because of a %s exception (%s)", typeid(e).name(), e.what());
    }

    Action outcome = Action::END;
    if (step_index < s.steps.size()) {
      const Step& step = s.steps[step_index];
      assert(step.entity == Step::SND && step.type == Step::ACTION);
      outcome = step.action_type;
    }

    if (outcome != expected) {
      XBT_ERROR("Expected %s but got %s in %s", to_string(expected), to_string(outcome), scenario_string.c_str());
    } else {
      XBT_DEBUG("OK: %s", scenario_string.c_str());
    }
    sg4::this_actor::sleep_until(end_time);

    xbt_assert(not mbox_eager->listen(), "Mailbox should not have ongoing communication!");
    xbt_assert(not mbox_rdv->listen(), "Mailbox should not have ongoing communication!");

  }

public:
  explicit SendAgent(int id, sg4::Host* other_host) : id(id), other_host(other_host) {}

  void operator()()
  {
    run++;
    XBT_DEBUG("Host %i starts run %i and scenario %lu.", id, run, scenario);
    while (scenario < scenarios.size()) {
      Scenario& s = scenarios[scenario];
      scenario++;
      send_message(s);
    }
  }
};

int SendAgent::run         = 0;
size_t SendAgent::scenario = 0;

/*************************************************************************************************/

class ReceiveAgent {

  static int run;
  static size_t scenario;
  int id;
  sg4::Host* other_host;

  sg4::CommPtr do_get(CommType type, double*& receive_ptr)
  {
    switch (type) {
      case CommType::EAGER_SYNC:
        receive_ptr = mbox_eager->get<double>();
        return nullptr;
      case CommType::EAGER_ASYNC:
        return mbox_eager->get_async(&receive_ptr);
      case CommType::EAGER_INIT:
        return mbox_eager->get_init()->set_dst_data((void**)(&receive_ptr));
      case CommType::RDV_SYNC:
        receive_ptr = mbox_rdv->get<double>();
        return nullptr;
      case CommType::RDV_ASYNC:
        return mbox_rdv->get_async(&receive_ptr);
      case CommType::RDV_INIT:
        return mbox_rdv->get_init()->set_dst_data((void**)(&receive_ptr));
      case CommType::ONESIDE_SYNC:
      case CommType::ONESIDE_ASYNC:
        xbt_die("No get in One Sided comunications!");
    }
    return nullptr;
  }

  void receive_message(const Scenario& s)
  {
    sg4::CommPtr comm = nullptr;
    CommType type     = s.type;
    Action expected   = s.rcv_expected;
    double end_time   = s.start_time + s.duration;
    double* receive_ptr = nullptr;
    size_t step_index   = 0;
    sg4::this_actor::sleep_until(s.start_time);

    //Make sure we have a clean slate
    xbt_assert(not mbox_eager->listen(),"Eager mailbox should be empty when starting a test");
    xbt_assert(not mbox_rdv->listen(),"RDV mailbox should be empty when starting a test");

    for (; step_index < s.steps.size(); step_index++) {
      const Step& step = s.steps[step_index];
      if (step.entity != Step::RCV || step.type != Step::ACTION)
        continue;

      try {
        sg4::this_actor::sleep_until(s.start_time + step.rel_time);
      } catch (std::exception& e) {
        XBT_DEBUG("During Sleep, failed to receive message because of a %s exception (%s)", typeid(e).name(), e.what());
        break;
      }

      // Check if the other host is still OK.
      if (not other_host->is_on())
        break;

      // Perform the action
      try {
        switch (step.action_type) {
          case Action::GET:
            comm = do_get(type, receive_ptr);
            break;
          case Action::START:
            comm->start();
            break;
          case Action::WAIT:
            comm->wait();
            break;
          default:
            xbt_die("Not a valid action for RCV");
        }
      } catch (std::exception& e) {
        XBT_DEBUG("During %s, failed to receive message because of a %s exception (%s)", to_string(step.action_type),
                  typeid(e).name(), e.what());
        break;
      }
    }

    try {
      sg4::this_actor::sleep_until(end_time - .1);
    } catch (std::exception& e) {
      XBT_DEBUG("During Sleep, failed to send message because of a %s exception (%s)", typeid(e).name(), e.what());
    }

    Action outcome              = Action::END;
    std::string scenario_string = to_string(s);
    if (step_index < s.steps.size()) {
      const Step& step = s.steps[step_index];
      assert(step.entity == Step::RCV && step.type == Step::ACTION);
      outcome = step.action_type;
    } else if (s.type!=CommType::ONESIDE_SYNC && 
          s.type!=CommType::ONESIDE_ASYNC
          ) {
            //One sided / detached operations do not actually transfer anything
            if(receive_ptr == nullptr ) {
              XBT_ERROR("Received address is NULL in %s", scenario_string.c_str());
            } else if (*receive_ptr != end_time) {
              XBT_ERROR("Received value invalid: expected %f but got %f in %s", end_time, *receive_ptr,
                scenario_string.c_str());
          }
    }

    if (outcome != expected) {
      XBT_ERROR("Expected %s but got %s in %s", to_string(expected), to_string(outcome), scenario_string.c_str());
    } else {
      XBT_DEBUG("OK: %s", scenario_string.c_str());
    }

    sg4::this_actor::sleep_until(end_time);
    xbt_assert(not mbox_eager->listen(), "Mailbox should not have ongoing communication!");
    xbt_assert(not mbox_rdv->listen(), "Mailbox should not have ongoing communication!");
  }

public:
  explicit ReceiveAgent(int id, sg4::Host* other_host) : id(id), other_host(other_host) {}

  void operator()()
  {
    run++;
    XBT_DEBUG("Host %i starts run %i and scenario %lu.", id, run, scenario);
    mbox_eager->set_receiver(sg4::Actor::self());
    while (scenario < scenarios.size()) {
      Scenario& s = scenarios[scenario];
      scenario++;
      receive_message(s);
    }
  }
};

int ReceiveAgent::run         = 0;
size_t ReceiveAgent::scenario = 0;

/*************************************************************************************************/

static void on_host_state_change(sg4::Host const& host)
{
  XBT_DEBUG("Host %s is now %s", host.get_cname(), host.is_on() ? "ON " : "OFF");
  if(not host.is_on()) {
    mbox_eager->clear();
    mbox_rdv->clear();
  }
}

static void on_link_state_change(sg4::Link const& link)
{
  XBT_DEBUG("Link %s is now %s", link.get_cname(), link.is_on() ? "ON " : "OFF");
}

static void addStateEvent(std::ostream& out, double date, bool isOn)
{
  if (isOn)
    out << date << " 1\n";
  else
    out << date << " 0\n";
}

static int prepareScenario(CommType type, int& index, double& start_time, double duration, std::ostream& sndP, std::ostream& rcvP,
                            std::ostream& lnkP, Action snd_expected, Action rcv_expected, std::vector<Step> steps, std::vector<int> active_indices)
{
  int ret=0;
  if(std::find(active_indices.begin(),active_indices.end(),index)!=active_indices.end()) {
    // Update fault profiles
    for (Step& step : steps) {
      assert(step.rel_time < duration);
      if (step.type != Step::STATE)
        continue;
      int val = step.new_state ? 1 : 0;
      switch (step.entity) {
        case Step::SND:
          sndP << start_time + step.rel_time << " " << val << std::endl;
          break;
        case Step::RCV:
          rcvP << start_time + step.rel_time << " " << val << std::endl;
          break;
        case Step::LNK:
          lnkP << start_time + step.rel_time << " " << val << std::endl;
          break;
      }
    }
    scenarios.push_back({type, start_time, duration, snd_expected, rcv_expected, steps, index}); 
    ret=1;
  }
  index++;
  start_time += duration;
  return ret;
}

/*************************************************************************************************/

// State profiles for the resources
std::string SndStateProfile;
std::string RcvStateProfile;
std::string LnkStateProfile;

/*************************************************************************************************/

double build_scenarios(std::vector<int>& active_indices );

int main(int argc, char* argv[])
{

  sg4::Engine e(&argc, argv);

  std::vector<int> active_indices;
  //Parse index of tests that need to be run

  int previous_index=-1;
  bool is_range_last=false;
  for(int i=1; i<argc; i++) {
    if(not strcmp(argv[i],"-"))
      is_range_last=true;
    else {
      int index=atoi(argv[i]);
      xbt_assert(index>previous_index);
      if(is_range_last) 
        for(int j=previous_index+1;j<=index;j++)
          active_indices.push_back(j);
      else
         active_indices.push_back(index);
       is_range_last=false;
       previous_index=index;
    }
  }

  double end_time = build_scenarios(active_indices);

  XBT_INFO("Will run for %f seconds", end_time);


  mbox_eager = e.mailbox_by_name_or_create("eager");
  mbox_rdv   = e.mailbox_by_name_or_create("rdv");
  sg4::NetZone* zone = sg4::create_full_zone("Top");
  pr::Profile* profile_sender = pr::ProfileBuilder::from_string("sender_profile", SndStateProfile, 0);
  sg4::Host* sender_host = zone->create_host("senderHost", HostComputePower)->set_state_profile(profile_sender)->seal();
  pr::Profile* profile_receiver = pr::ProfileBuilder::from_string("receiver_profile", RcvStateProfile, 0);
  sg4::Host* receiver_host = zone->create_host("receiverHost", HostComputePower)->set_state_profile(profile_receiver)->seal();
  sg4::ActorPtr sender = sg4::Actor::create("sender", sender_host, SendAgent(0, receiver_host));
  sender->set_auto_restart(true);
  sg4::ActorPtr receiver = sg4::Actor::create("receiver", receiver_host, ReceiveAgent(1, sender_host));
  receiver->set_auto_restart(true);
  pr::Profile* profile_link = pr::ProfileBuilder::from_string("link_profile", LnkStateProfile, 0);
  sg4::Link* link =
      zone->create_link("link", LinkBandwidth)->set_latency(LinkLatency)->set_state_profile(profile_link)->seal();
  zone->add_route(sender_host->get_netpoint(), receiver_host->get_netpoint(), nullptr, nullptr,
                  {sg4::LinkInRoute{link}}, false);
  zone->seal();
  sg4::Host::on_state_change.connect(on_host_state_change);
  sg4::Link::on_state_change_cb(on_link_state_change);

  e.run_until(end_time);

  //Make sure we have a clean slate
  xbt_assert(not mbox_eager->listen(),"Eager mailbox should be empty in the end");
  xbt_assert(not mbox_rdv->listen(),"RDV mailbox should be empty in the end");
  return 0;
}

/*************************************************************************************************/

// A bunch of dirty macros to help readability (supposedly)
#define _MK(type, duration, snd_expected, rcv_expected, steps...)                                                      \
  active+=prepareScenario(CommType::type, index, start_time, duration, sndP, rcvP, lnkP, Action::snd_expected, Action::rcv_expected,  \
                  {steps}, active_indices )
// Link
#define LOFF(relTime)                                                                                                  \
  {                                                                                                                    \
    relTime, Step::STATE, Step::LNK, Action::END, false                                                                \
  }
#define LON(relTime)                                                                                                   \
  {                                                                                                                    \
    relTime, Step::STATE, Step::LNK, Action::END, true                                                                 \
  }
// Sender
#define SOFF(relTime)                                                                                                  \
  {                                                                                                                    \
    relTime, Step::STATE, Step::SND, Action::END, false                                                                \
  }
#define SON(relTime)                                                                                                   \
  {                                                                                                                    \
    relTime, Step::STATE, Step::SND, Action::END, true                                                                 \
  }
#define SPUT(relTime)                                                                                                  \
  {                                                                                                                    \
    relTime, Step::ACTION, Step::SND, Action::PUT, false                                                               \
  }
#define SWAT(relTime)                                                                                                  \
  {                                                                                                                    \
    relTime, Step::ACTION, Step::SND, Action::WAIT, false                                                              \
  }
// Receiver
#define ROFF(relTime)                                                                                                  \
  {                                                                                                                    \
    relTime, Step::STATE, Step::RCV, Action::END, false                                                                \
  }
#define RON(relTime)                                                                                                   \
  {                                                                                                                    \
    relTime, Step::STATE, Step::RCV, Action::END, true                                                                 \
  }
#define RGET(relTime)                                                                                                  \
  {                                                                                                                    \
    relTime, Step::ACTION, Step::RCV, Action::GET, false                                                               \
  }
#define RWAT(relTime)                                                                                                  \
  {                                                                                                                    \
    relTime, Step::ACTION, Step::RCV, Action::WAIT, false                                                              \
  }

double build_scenarios(std::vector<int>& active_indices )
{

  // Build the set of simulation stages
  std::stringstream sndP;
  std::stringstream rcvP;
  std::stringstream lnkP;

  double start_time = 0;
  int index=0;
  int active=0;

  // EAGER SYNC use cases
  // All good
  _MK(EAGER_SYNC, 1, END, END, SPUT(.2), RGET(.4));
  _MK(EAGER_SYNC, 1, END, END, RGET(.2), SPUT(.4));
  // Receiver off
  _MK(EAGER_SYNC, 2, PUT, DIE, ROFF(.1), SPUT(.2), RON(1));
  _MK(EAGER_SYNC, 2, PUT, DIE, SPUT(.2), ROFF(.3), RON(1));
  _MK(EAGER_SYNC, 2, PUT, DIE, SPUT(.2), RGET(.4), ROFF(.5), RON(1));
  _MK(EAGER_SYNC, 2, PUT, DIE, RGET(.2), SPUT(.4), ROFF(.5), RON(1));
  // Sender off
  _MK(EAGER_SYNC, 2, DIE, GET, SPUT(.2), SOFF(.3), RGET(.4), SON(1));
  _MK(EAGER_SYNC, 2, DIE, GET, SPUT(.2), RGET(.4), SOFF(.5), SON(1));
  // Link off
  _MK(EAGER_SYNC, 2, PUT, GET, LOFF(.1), SPUT(.2), RGET(.4), LON(1));
  _MK(EAGER_SYNC, 2, PUT, GET, SPUT(.2), LOFF(.3), RGET(.4), LON(1));
  _MK(EAGER_SYNC, 2, PUT, GET, SPUT(.2), RGET(.4), LOFF(.5), LON(1));
  _MK(EAGER_SYNC, 2, PUT, GET, LOFF(.1), RGET(.2), SPUT(.4), LON(1));
  _MK(EAGER_SYNC, 2, PUT, GET, RGET(.2), LOFF(.3), SPUT(.4), LON(1));
  _MK(EAGER_SYNC, 2, PUT, GET, RGET(.2), SPUT(.4), LOFF(.5), LON(1));

  // EAGER ASYNC use cases
  // All good
  _MK(EAGER_ASYNC, 2, END, END, SPUT(.2), SWAT(.4), RGET(.6), RWAT(.8));
  _MK(EAGER_ASYNC, 2, END, END, SPUT(.2), RGET(.4), SWAT(.6), RWAT(.8));
  _MK(EAGER_ASYNC, 2, END, END, SPUT(.2), RGET(.4), RWAT(.6), SWAT(.8));
  _MK(EAGER_ASYNC, 2, END, END, RGET(.2), SPUT(.4), SWAT(.6), RWAT(.8));
  _MK(EAGER_ASYNC, 2, END, END, RGET(.2), SPUT(.4), RWAT(.6), SWAT(.8));
  _MK(EAGER_ASYNC, 2, END, END, RGET(.2), RWAT(.4), SPUT(.6), SWAT(.8));
  
  // Receiver off
  _MK(EAGER_ASYNC, 2, PUT, DIE, ROFF(.1), SPUT(.2), SWAT(.4), RON(1));
  _MK(EAGER_ASYNC, 2, WAIT, DIE, SPUT(.2), ROFF(.3), SWAT(.4), RON(1));
  _MK(EAGER_ASYNC, 2, PUT, DIE, RGET(.2), ROFF(.3), SPUT(.4), SWAT(.6), RON(1));
  _MK(EAGER_ASYNC, 2, WAIT, DIE, SPUT(.2), SWAT(.4), ROFF(.5), RON(1));
  _MK(EAGER_ASYNC, 2, WAIT, DIE, SPUT(.2), RGET(.4), ROFF(.5), SWAT(.6), RON(1));
  _MK(EAGER_ASYNC, 2, WAIT, DIE, RGET(.2), SPUT(.4), ROFF(.5), SWAT(.6), RON(1));
  _MK(EAGER_ASYNC, 2, PUT , DIE, RGET(.2), RWAT(.4), ROFF(.5), SPUT(.6), SWAT(.8), RON(1));
  _MK(EAGER_ASYNC, 2, WAIT, DIE, SPUT(.2), SWAT(.4), RGET(.6), ROFF(.7), RON(1));
  _MK(EAGER_ASYNC, 2, WAIT, DIE, SPUT(.2), RGET(.4), SWAT(.6), ROFF(.7), RON(1));
  _MK(EAGER_ASYNC, 2, WAIT, DIE, SPUT(.2), RGET(.4), RWAT(.6), ROFF(.7), SWAT(.8), RON(1));
  _MK(EAGER_ASYNC, 2, WAIT, DIE, RGET(.2), SPUT(.4), SWAT(.6), ROFF(.7), RON(1));
  _MK(EAGER_ASYNC, 2, WAIT, DIE, RGET(.2), SPUT(.4), RWAT(.6), ROFF(.7), SWAT(.8), RON(1));
  _MK(EAGER_ASYNC, 2, WAIT, DIE, RGET(.2), RWAT(.4), SPUT(.6), ROFF(.7), SWAT(.8), RON(1));
  // Sender off (only cases where sender did put, because otherwise receiver cannot find out there was a fault)
  _MK(EAGER_ASYNC, 2, DIE, GET, SPUT(.2), SOFF(.3), RGET(.4), RWAT(.6), SON(1));
  _MK(EAGER_ASYNC, 2, DIE, WAIT, RGET(.2), SPUT(.4), SOFF(.5), RWAT(.6), SON(1));
  _MK(EAGER_ASYNC, 2, DIE, WAIT, SPUT(.2), RGET(.4), SOFF(.5), RWAT(.6), SON(1));
  _MK(EAGER_ASYNC, 2, DIE, GET, SPUT(.2), SWAT(.4), SOFF(.5), RGET(.6), RWAT(.8), SON(1));
  _MK(EAGER_ASYNC, 2, DIE, WAIT, RGET(.2), RWAT(.4), SPUT(.6), SOFF(.7), SON(1));
  _MK(EAGER_ASYNC, 2, DIE, WAIT, RGET(.2), SPUT(.4), RWAT(.6), SOFF(.7), SON(1));
  _MK(EAGER_ASYNC, 2, DIE, WAIT, RGET(.2), SPUT(.4), SWAT(.6), SOFF(.7), RWAT(.8), SON(1));
  _MK(EAGER_ASYNC, 2, DIE, WAIT, SPUT(.2), RGET(.4), RWAT(.6), SOFF(.7), SON(1));
  _MK(EAGER_ASYNC, 2, DIE, WAIT, SPUT(.2), RGET(.4), SWAT(.6), SOFF(.7), RWAT(.8), SON(1));
  _MK(EAGER_ASYNC, 2, DIE, WAIT, SPUT(.2), SWAT(.4), RGET(.6), SOFF(.7), RWAT(.8), SON(1));
  // Link off
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, LOFF(.1), SPUT(.2), SWAT(.4), RGET(.6), RWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, LOFF(.1), SPUT(.2), RGET(.4), SWAT(.6), RWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, LOFF(.1), SPUT(.2), RGET(.4), RWAT(.6), SWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, LOFF(.1), RGET(.2), SPUT(.4), SWAT(.6), RWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, LOFF(.1), RGET(.2), SPUT(.4), RWAT(.6), SWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, LOFF(.1), RGET(.2), RWAT(.4), SPUT(.6), SWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, SPUT(.2), LOFF(.3), SWAT(.4), RGET(.6), RWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, SPUT(.2), LOFF(.3), RGET(.4), SWAT(.6), RWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, SPUT(.2), LOFF(.3), RGET(.4), RWAT(.6), SWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, RGET(.2), LOFF(.3), SPUT(.4), SWAT(.6), RWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, RGET(.2), LOFF(.3), SPUT(.4), RWAT(.6), SWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, RGET(.2), LOFF(.3), RWAT(.4), SPUT(.6), SWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, SPUT(.2), SWAT(.4), LOFF(.5), RGET(.6), RWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, SPUT(.2), RGET(.4), LOFF(.5), SWAT(.6), RWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, SPUT(.2), RGET(.4), LOFF(.5), RWAT(.6), SWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, RGET(.2), SPUT(.4), LOFF(.5), SWAT(.6), RWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, RGET(.2), SPUT(.4), LOFF(.5), RWAT(.6), SWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, RGET(.2), RWAT(.4), LOFF(.5), SPUT(.6), SWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, SPUT(.2), SWAT(.4), RGET(.6), LOFF(.7), RWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, SPUT(.2), RGET(.4), SWAT(.6), LOFF(.7), RWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, SPUT(.2), RGET(.4), RWAT(.6), LOFF(.7), SWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, RGET(.2), SPUT(.4), SWAT(.6), LOFF(.7), RWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, RGET(.2), SPUT(.4), RWAT(.6), LOFF(.7), SWAT(.8), LON(1));
  _MK(EAGER_ASYNC, 2, WAIT, WAIT, RGET(.2), RWAT(.4), SPUT(.6), LOFF(.7), SWAT(.8), LON(1));

  // RDV SYNC use cases

  // All good
  _MK(RDV_SYNC, 1, END, END, SPUT(.2), RGET(.4));
  _MK(RDV_SYNC, 1, END, END, RGET(.2), SPUT(.4));
  
  // Receiver off
  _MK(RDV_SYNC, 2, PUT, DIE, ROFF(.1), SPUT(.2), RON(1));
  _MK(RDV_SYNC, 2, PUT, DIE, SPUT(.2), ROFF(.3), RON(1)); //Fails because put comm cancellation does not trigger sender exception
  _MK(RDV_SYNC, 2, PUT, DIE, SPUT(.2), RGET(.4), ROFF(.5), RON(1));
  _MK(RDV_SYNC, 2, PUT, DIE, RGET(.2), SPUT(.4), ROFF(.5), RON(1));
  // Sender off
  _MK(RDV_SYNC, 2, DIE, GET, SPUT(.2), RGET(.4), SOFF(.5), SON(1));
  // Link off
  _MK(RDV_SYNC, 2, PUT, GET, LOFF(.1), SPUT(.2), RGET(.4), LON(1));
  _MK(RDV_SYNC, 2, PUT, GET, SPUT(.2), LOFF(.3), RGET(.4), LON(1));
  _MK(RDV_SYNC, 2, PUT, GET, SPUT(.2), RGET(.4), LOFF(.5), LON(1));
  _MK(RDV_SYNC, 2, PUT, GET, LOFF(.1), RGET(.2), SPUT(.4), LON(1));
  _MK(RDV_SYNC, 2, PUT, GET, RGET(.2), LOFF(.3), SPUT(.4), LON(1));
  _MK(RDV_SYNC, 2, PUT, GET, RGET(.2), SPUT(.4), LOFF(.5), LON(1));

  // RDV ASYNC use cases
  // All good
  _MK(RDV_ASYNC, 2, END, END, SPUT(.2), SWAT(.4), RGET(.6), RWAT(.8));
  _MK(RDV_ASYNC, 2, END, END, SPUT(.2), RGET(.4), SWAT(.6), RWAT(.8));
  _MK(RDV_ASYNC, 2, END, END, SPUT(.2), RGET(.4), RWAT(.6), SWAT(.8));
  _MK(RDV_ASYNC, 2, END, END, RGET(.2), SPUT(.4), SWAT(.6), RWAT(.8));
  _MK(RDV_ASYNC, 2, END, END, RGET(.2), SPUT(.4), RWAT(.6), SWAT(.8));
  _MK(RDV_ASYNC, 2, END, END, RGET(.2), RWAT(.4), SPUT(.6), SWAT(.8));
  // Receiver off
  _MK(RDV_ASYNC, 2, PUT, DIE, ROFF(.1), SPUT(.2), SWAT(.4), RON(1));
  _MK(RDV_ASYNC, 2, WAIT, DIE, SPUT(.2), ROFF(.3), SWAT(.4), RON(1));
  _MK(RDV_ASYNC, 2, PUT, DIE, RGET(.2), ROFF(.3), SPUT(.4), SWAT(.6), RON(1));
  _MK(RDV_ASYNC, 2, WAIT, DIE, SPUT(.2), SWAT(.4), ROFF(.5), RON(1)); //Fails because put comm cancellation does not trigger sender exception
  _MK(RDV_ASYNC, 2, WAIT, DIE, SPUT(.2), RGET(.4), ROFF(.5), SWAT(.6), RON(1));
  _MK(RDV_ASYNC, 2, WAIT, DIE, RGET(.2), SPUT(.4), ROFF(.5), SWAT(.6), RON(1));
  _MK(RDV_ASYNC, 2, PUT , DIE, RGET(.2), RWAT(.4), ROFF(.5), SPUT(.6), SWAT(.8), RON(1));
  _MK(RDV_ASYNC, 2, WAIT, DIE, SPUT(.2), SWAT(.4), RGET(.6), ROFF(.7), RON(1));
  _MK(RDV_ASYNC, 2, WAIT, DIE, SPUT(.2), RGET(.4), SWAT(.6), ROFF(.7), RON(1));
  _MK(RDV_ASYNC, 2, WAIT, DIE, SPUT(.2), RGET(.4), RWAT(.6), ROFF(.7), SWAT(.8), RON(1));
  _MK(RDV_ASYNC, 2, WAIT, DIE, RGET(.2), SPUT(.4), SWAT(.6), ROFF(.7), RON(1));
  _MK(RDV_ASYNC, 2, WAIT, DIE, RGET(.2), SPUT(.4), RWAT(.6), ROFF(.7), SWAT(.8), RON(1));
  _MK(RDV_ASYNC, 2, WAIT, DIE, RGET(.2), RWAT(.4), SPUT(.6), ROFF(.7), SWAT(.8), RON(1));
  // Sender off (only cases where sender did put, because otherwise receiver cannot find out there was a fault)
  _MK(RDV_ASYNC, 2, DIE, GET, SPUT(.2), SOFF(.3), RGET(.4), RWAT(.6), SON(1));
  _MK(RDV_ASYNC, 2, DIE, WAIT, RGET(.2), SPUT(.4), SOFF(.5), RWAT(.6), SON(1));
  _MK(RDV_ASYNC, 2, DIE, WAIT, SPUT(.2), RGET(.4), SOFF(.5), RWAT(.6), SON(1));
  _MK(RDV_ASYNC, 2, DIE, GET, SPUT(.2), SWAT(.4), SOFF(.5), RGET(.6), RWAT(.8), SON(1));
  _MK(RDV_ASYNC, 2, DIE, WAIT, RGET(.2), RWAT(.4), SPUT(.6), SOFF(.7), SON(1));
  _MK(RDV_ASYNC, 2, DIE, WAIT, RGET(.2), SPUT(.4), RWAT(.6), SOFF(.7), SON(1));
  _MK(RDV_ASYNC, 2, DIE, WAIT, RGET(.2), SPUT(.4), SWAT(.6), SOFF(.7), RWAT(.8), SON(1));
  _MK(RDV_ASYNC, 2, DIE, WAIT, SPUT(.2), RGET(.4), RWAT(.6), SOFF(.7), SON(1));
  _MK(RDV_ASYNC, 2, DIE, WAIT, SPUT(.2), RGET(.4), SWAT(.6), SOFF(.7), RWAT(.8), SON(1));
  _MK(RDV_ASYNC, 2, DIE, WAIT, SPUT(.2), SWAT(.4), RGET(.6), SOFF(.7), RWAT(.8), SON(1));
  // Link off
  _MK(RDV_ASYNC, 2, WAIT, WAIT, LOFF(.1), SPUT(.2), SWAT(.4), RGET(.6), RWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, LOFF(.1), SPUT(.2), RGET(.4), SWAT(.6), RWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, LOFF(.1), SPUT(.2), RGET(.4), RWAT(.6), SWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, LOFF(.1), RGET(.2), SPUT(.4), SWAT(.6), RWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, LOFF(.1), RGET(.2), SPUT(.4), RWAT(.6), SWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, LOFF(.1), RGET(.2), RWAT(.4), SPUT(.6), SWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, SPUT(.2), LOFF(.3), SWAT(.4), RGET(.6), RWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, SPUT(.2), LOFF(.3), RGET(.4), SWAT(.6), RWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, SPUT(.2), LOFF(.3), RGET(.4), RWAT(.6), SWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, RGET(.2), LOFF(.3), SPUT(.4), SWAT(.6), RWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, RGET(.2), LOFF(.3), SPUT(.4), RWAT(.6), SWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, RGET(.2), LOFF(.3), RWAT(.4), SPUT(.6), SWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, SPUT(.2), SWAT(.4), LOFF(.5), RGET(.6), RWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, SPUT(.2), RGET(.4), LOFF(.5), SWAT(.6), RWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, SPUT(.2), RGET(.4), LOFF(.5), RWAT(.6), SWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, RGET(.2), SPUT(.4), LOFF(.5), SWAT(.6), RWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, RGET(.2), SPUT(.4), LOFF(.5), RWAT(.6), SWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, RGET(.2), RWAT(.4), LOFF(.5), SPUT(.6), SWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, SPUT(.2), SWAT(.4), RGET(.6), LOFF(.7), RWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, SPUT(.2), RGET(.4), SWAT(.6), LOFF(.7), RWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, SPUT(.2), RGET(.4), RWAT(.6), LOFF(.7), SWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, RGET(.2), SPUT(.4), SWAT(.6), LOFF(.7), RWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, RGET(.2), SPUT(.4), RWAT(.6), LOFF(.7), SWAT(.8), LON(1));
  _MK(RDV_ASYNC, 2, WAIT, WAIT, RGET(.2), RWAT(.4), SPUT(.6), LOFF(.7), SWAT(.8), LON(1));

  // ONESIDE SYNC use cases

  // All good
  _MK(ONESIDE_SYNC, 1, END, END, SPUT(.2));
  // Receiver off
  _MK(ONESIDE_SYNC, 2, PUT, DIE, ROFF(.1), SPUT(.2), RON(1));
  _MK(ONESIDE_SYNC, 2, PUT, DIE, SPUT(.2), ROFF(.3), RON(1));
  // Sender off
  _MK(ONESIDE_SYNC, 2, DIE, END, SPUT(.2), SOFF(.3), SON(1));
  // Link off
  _MK(ONESIDE_SYNC, 2, PUT, END, LOFF(.1), SPUT(.2), LON(1));
  _MK(ONESIDE_SYNC, 2, PUT, END, SPUT(.2), LOFF(.3), LON(1));

  // ONESIDE ASYNC use cases
  // All good
  _MK(ONESIDE_ASYNC, 2, END, END, SPUT(.2), SWAT(.4));
  // Receiver off
  _MK(ONESIDE_ASYNC, 2, PUT, DIE, ROFF(.1), SPUT(.2), SWAT(.4), RON(1));
  _MK(ONESIDE_ASYNC, 2, WAIT, DIE, SPUT(.2), ROFF(.3), SWAT(.4), RON(1));
  _MK(ONESIDE_ASYNC, 2, WAIT, DIE, SPUT(.2), SWAT(.4), ROFF(.5), RON(1));
  // Sender off
  _MK(ONESIDE_ASYNC, 2, DIE, END, SPUT(.2), SOFF(.3), SON(1));
  // Link off
  _MK(ONESIDE_ASYNC, 2, WAIT, END, LOFF(.1), SPUT(.2), SWAT(.4), LON(1));
  _MK(ONESIDE_ASYNC, 2, WAIT, END, SPUT(.2), LOFF(.3), SWAT(.4), LON(1));
  _MK(ONESIDE_ASYNC, 2, WAIT, END, SPUT(.2), SWAT(.4), LOFF(.5), LON(1));


  SndStateProfile = sndP.str();
  RcvStateProfile = rcvP.str();
  LnkStateProfile = lnkP.str();

  XBT_INFO("Will execute %i active scenarios out of %i.",active,index);
  return start_time + 1;
}
