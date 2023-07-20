/* Copyright (c) 2010-2023. The SimGrid Team. All rights reserved.          */

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
 * Essentially, it catches exceptions related to communications and it clears the mailboxes when one of the nodes gets
 * turned off. However, this model would suppose that there would be 2 mailboxes for each pair of nodes, which is
 * probably unacceptable.
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
constexpr auto MsgSize = static_cast<uint64_t>(LinkBandwidth / 2);

/*************************************************************************************************/

XBT_DECLARE_ENUM_CLASS(CommType, EAGER_SYNC, EAGER_ASYNC, EAGER_INIT, RDV_SYNC, RDV_ASYNC, RDV_INIT, ONESIDE_SYNC,
                       ONESIDE_ASYNC
                       // ONESIDE_INIT is equivalent to ONESIDE_ASYNC
);

XBT_DECLARE_ENUM_CLASS(Action, SLEEP, PUT, GET, START, WAIT, DIE, END);

struct Step {
  XBT_DECLARE_ENUM_CLASS(Type, STATE, ACTION);
  XBT_DECLARE_ENUM_CLASS(Entity, LNK, SND, RCV);

  double rel_time; // Time relative to Scenario startTime
  Type type;
  Entity entity;
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
  ss << "#" << s.index << "[" << s.start_time << "s," << s.start_time + s.duration << "s[: (" << to_c_str(s.type);
  ss << ") Expected: S:" << to_c_str(s.snd_expected) << " R:" << to_c_str(s.rcv_expected) << " Steps: ";
  for (const Step& step : s.steps) {
    ss << "+" << step.rel_time << "s:" << Step::to_c_str(step.entity);

    if (step.type == Step::Type::STATE) {
      ss << "->";
      if (step.new_state)
        ss << "ON";
      else
        ss << "OFF";
    } else {
      ss << "." << to_c_str(step.action_type);
    }
    ss << " ";
  }
  return ss.str();
}

struct MBoxes {
  sg4::Mailbox* eager;
  sg4::Mailbox* rdv;
};

struct ScenarioContext {
  int index;
  int active;
  double start_time;
  std::stringstream sender_profile;
  std::stringstream receiver_profile;
  std::stringstream link_profile;
  std::vector<int> active_indices;
  std::vector<Scenario> scenarios;
};

class SendAgent {
  static int run_;
  static size_t scenario_;
  int id_;
  sg4::Host* other_host_;
  const MBoxes& mbox_;
  const ScenarioContext& ctx_;

  sg4::CommPtr do_put(CommType type, double& send_value) const
  {
    switch (type) {
      case CommType::EAGER_SYNC:
        mbox_.eager->put(&send_value, MsgSize);
        return nullptr;
      case CommType::EAGER_ASYNC:
        return mbox_.eager->put_async(&send_value, MsgSize);
      case CommType::EAGER_INIT:
        return mbox_.eager->put_init(&send_value, MsgSize);
      case CommType::RDV_SYNC:
        mbox_.rdv->put(&send_value, MsgSize);
        return nullptr;
      case CommType::RDV_ASYNC:
        return mbox_.rdv->put_async(&send_value, MsgSize);
      case CommType::RDV_INIT:
        return mbox_.rdv->put_init(&send_value, MsgSize);
      case CommType::ONESIDE_SYNC:
        sg4::Comm::sendto(sg4::this_actor::get_host(), other_host_, MsgSize);
        return nullptr;
      case CommType::ONESIDE_ASYNC:
        return sg4::Comm::sendto_async(sg4::this_actor::get_host(), other_host_, MsgSize);
      default:
        break;
    }
    DIE_IMPOSSIBLE;
  }

  void send_message(const Scenario& s) const
  {
    std::string scenario_string = to_string(s);
    XBT_DEBUG("Will try: %s", scenario_string.c_str());
    double send_value;
    sg4::CommPtr comm = nullptr;
    Action expected   = s.snd_expected;
    double end_time   = s.start_time + s.duration;
    send_value        = end_time;
    size_t step_index = 0;
    sg4::this_actor::sleep_until(s.start_time);
    // Make sure we have a clean slate
    xbt_assert(not mbox_.eager->listen(), "Eager mailbox should be empty when starting a test");
    xbt_assert(not mbox_.rdv->listen(), "RDV mailbox should be empty when starting a test");

    Action current_action;
    try {
      for (; step_index < s.steps.size(); step_index++) {
        const Step& step = s.steps[step_index];
        if (step.entity != Step::Entity::SND || step.type != Step::Type::ACTION)
          continue;

        current_action = Action::SLEEP;
        sg4::this_actor::sleep_until(s.start_time + step.rel_time);

        // Check if the other host is still OK.
        if (not other_host_->is_on())
          break;

        // Perform the action
        current_action = step.action_type;
        switch (step.action_type) {
          case Action::PUT:
            comm = do_put(s.type, send_value);
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
      }
    } catch (const simgrid::Exception& e) {
      XBT_DEBUG("During %s, failed to send message because of a %s exception (%s)", to_c_str(current_action),
                typeid(e).name(), e.what());
    }
    try {
      sg4::this_actor::sleep_until(end_time);
    } catch (const simgrid::Exception& e) {
      XBT_DEBUG("During Sleep, failed to send message because of a %s exception (%s)", typeid(e).name(), e.what());
    }
    Action outcome = Action::END;
    if (step_index < s.steps.size()) {
      const Step& step = s.steps[step_index];
      assert(step.entity == Step::Entity::SND && step.type == Step::Type::ACTION);
      outcome = step.action_type;
    }
    if (outcome != expected) {
      XBT_ERROR("Expected %s but got %s in %s", to_c_str(expected), to_c_str(outcome), scenario_string.c_str());
    } else {
      XBT_DEBUG("OK: %s", scenario_string.c_str());
    }
    sg4::this_actor::sleep_until(end_time);
    xbt_assert(not mbox_.eager->listen(), "Mailbox should not have ongoing communication!");
    xbt_assert(not mbox_.rdv->listen(), "Mailbox should not have ongoing communication!");
  }

public:
  explicit SendAgent(int id, sg4::Host* other_host, const MBoxes& mbox, const ScenarioContext& ctx)
      : id_(id), other_host_(other_host), mbox_(mbox), ctx_(ctx)
  {
  }

  void operator()() const
  {
    run_++;
    XBT_DEBUG("Host %i starts run %i and scenario %zu.", id_, run_, scenario_);
    while (scenario_ < ctx_.scenarios.size()) {
      const Scenario& s = ctx_.scenarios[scenario_];
      scenario_++;
      send_message(s);
    }
  }
};

int SendAgent::run_         = 0;
size_t SendAgent::scenario_ = 0;

/*************************************************************************************************/

class ReceiveAgent {
  static int run_;
  static size_t scenario_;
  int id_;
  sg4::Host* other_host_;
  const MBoxes& mbox_;
  const ScenarioContext& ctx_;

  sg4::CommPtr do_get(CommType type, double*& receive_ptr) const
  {
    switch (type) {
      case CommType::EAGER_SYNC:
        receive_ptr = mbox_.eager->get<double>();
        return nullptr;
      case CommType::EAGER_ASYNC:
        return mbox_.eager->get_async(&receive_ptr);
      case CommType::EAGER_INIT:
        return mbox_.eager->get_init()->set_dst_data((void**)(&receive_ptr));
      case CommType::RDV_SYNC:
        receive_ptr = mbox_.rdv->get<double>();
        return nullptr;
      case CommType::RDV_ASYNC:
        return mbox_.rdv->get_async(&receive_ptr);
      case CommType::RDV_INIT:
        return mbox_.rdv->get_init()->set_dst_data((void**)(&receive_ptr));
      case CommType::ONESIDE_SYNC:
      case CommType::ONESIDE_ASYNC:
        xbt_die("No get in One Sided comunications!");
      default:
        break;
    }
    DIE_IMPOSSIBLE;
  }

  void receive_message(const Scenario& s) const
  {
    sg4::CommPtr comm   = nullptr;
    CommType type       = s.type;
    Action expected     = s.rcv_expected;
    double end_time     = s.start_time + s.duration;
    double* receive_ptr = nullptr;
    size_t step_index   = 0;
    sg4::this_actor::sleep_until(s.start_time);
    // Make sure we have a clean slate
    xbt_assert(not mbox_.eager->listen(), "Eager mailbox should be empty when starting a test");
    xbt_assert(not mbox_.rdv->listen(), "RDV mailbox should be empty when starting a test");

    Action current_action;
    try {
      for (; step_index < s.steps.size(); step_index++) {
        const Step& step = s.steps[step_index];
        if (step.entity != Step::Entity::RCV || step.type != Step::Type::ACTION)
          continue;

        current_action = Action::SLEEP;
        sg4::this_actor::sleep_until(s.start_time + step.rel_time);

        // Check if the other host is still OK.
        if (not other_host_->is_on())
          break;
        // Perform the action
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
      }
    } catch (const simgrid::Exception& e) {
      XBT_DEBUG("During %s, failed to receive message because of a %s exception (%s)", to_c_str(current_action),
                typeid(e).name(), e.what());
    }
    try {
      sg4::this_actor::sleep_until(end_time - .1);
    } catch (const simgrid::Exception& e) {
      XBT_DEBUG("During Sleep, failed to send message because of a %s exception (%s)", typeid(e).name(), e.what());
    }
    Action outcome              = Action::END;
    std::string scenario_string = to_string(s);
    if (step_index < s.steps.size()) {
      const Step& step = s.steps[step_index];
      assert(step.entity == Step::Entity::RCV && step.type == Step::Type::ACTION);
      outcome = step.action_type;
    } else if (s.type != CommType::ONESIDE_SYNC && s.type != CommType::ONESIDE_ASYNC) {
      // One sided / detached operations do not actually transfer anything
      if (receive_ptr == nullptr) {
        XBT_ERROR("Received address is NULL in %s", scenario_string.c_str());
      } else if (*receive_ptr != end_time) {
        XBT_ERROR("Received value invalid: expected %f but got %f in %s", end_time, *receive_ptr,
                  scenario_string.c_str());
      }
    }
    if (outcome != expected) {
      XBT_ERROR("Expected %s but got %s in %s", to_c_str(expected), to_c_str(outcome), scenario_string.c_str());
    } else {
      XBT_DEBUG("OK: %s", scenario_string.c_str());
    }
    sg4::this_actor::sleep_until(end_time);
    xbt_assert(not mbox_.eager->listen(), "Mailbox should not have ongoing communication!");
    xbt_assert(not mbox_.rdv->listen(), "Mailbox should not have ongoing communication!");
  }

public:
  explicit ReceiveAgent(int id, sg4::Host* other_host, const MBoxes& mbox, const ScenarioContext& ctx)
      : id_(id), other_host_(other_host), mbox_(mbox), ctx_(ctx)
  {
  }
  void operator()() const
  {
    run_++;
    XBT_DEBUG("Host %i starts run %i and scenario %zu.", id_, run_, scenario_);
    mbox_.eager->set_receiver(sg4::Actor::self());
    while (scenario_ < ctx_.scenarios.size()) {
      const Scenario& s = ctx_.scenarios[scenario_];
      scenario_++;
      receive_message(s);
    }
  }
};

int ReceiveAgent::run_         = 0;
size_t ReceiveAgent::scenario_ = 0;

static double build_scenarios(ScenarioContext& ctx);

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  ScenarioContext ctx;
  int previous_index = -1;
  bool is_range_last = false;
  for (int i = 1; i < argc; i++) {
    if (not strcmp(argv[i], "-")) {
      is_range_last = true;
      continue;
    }
    int index = atoi(argv[i]);
    xbt_assert(index > previous_index);
    if (is_range_last)
      for (int j = previous_index + 1; j <= index; j++)
        ctx.active_indices.push_back(j);
    else
      ctx.active_indices.push_back(index);
    is_range_last  = false;
    previous_index = index;
  }
  double end_time = build_scenarios(ctx);
  XBT_INFO("Will run for %f seconds", end_time);
  MBoxes mbox;
  mbox.eager                  = e.mailbox_by_name_or_create("eager");
  mbox.rdv                    = e.mailbox_by_name_or_create("rdv");
  sg4::NetZone* zone          = sg4::create_full_zone("Top");
  pr::Profile* profile_sender = pr::ProfileBuilder::from_string("sender_profile", ctx.sender_profile.str(), 0);
  sg4::Host* sender_host = zone->create_host("senderHost", HostComputePower)->set_state_profile(profile_sender)->seal();
  pr::Profile* profile_receiver = pr::ProfileBuilder::from_string("receiver_profile", ctx.receiver_profile.str(), 0);
  sg4::Host* receiver_host =
      zone->create_host("receiverHost", HostComputePower)->set_state_profile(profile_receiver)->seal();
  sg4::ActorPtr sender = sg4::Actor::create("sender", sender_host, SendAgent(0, receiver_host, mbox, ctx));
  sender->set_auto_restart(true);
  sg4::ActorPtr receiver = sg4::Actor::create("receiver", receiver_host, ReceiveAgent(1, sender_host, mbox, ctx));
  receiver->set_auto_restart(true);
  pr::Profile* profile_link = pr::ProfileBuilder::from_string("link_profile", ctx.link_profile.str(), 0);
  sg4::Link const* link =
      zone->create_link("link", LinkBandwidth)->set_latency(LinkLatency)->set_state_profile(profile_link)->seal();
  zone->add_route(sender_host, receiver_host, {link});
  zone->seal();

  sg4::Host::on_onoff_cb([mbox](sg4::Host const& host) {
    XBT_DEBUG("Host %s is now %s", host.get_cname(), host.is_on() ? "ON " : "OFF");
    if (not host.is_on()) {
      mbox.eager->clear();
      mbox.rdv->clear();
    }
  });

  sg4::Link::on_onoff_cb(
      [](sg4::Link const& lnk) { XBT_DEBUG("Link %s is now %s", lnk.get_cname(), lnk.is_on() ? "ON " : "OFF"); });

  e.run_until(end_time);

  // Make sure we have a clean slate
  xbt_assert(not mbox.eager->listen(), "Eager mailbox should be empty in the end");
  xbt_assert(not mbox.rdv->listen(), "RDV mailbox should be empty in the end");
  XBT_INFO("Done.");
  return 0;
}

static void prepareScenario(ScenarioContext& ctx, CommType type, double duration, Action sender_expected,
                            Action receiver_expected, const std::vector<Step>& steps)
{
  if (std::find(ctx.active_indices.begin(), ctx.active_indices.end(), ctx.index) != ctx.active_indices.end()) {
    // Update fault profiles
    for (const Step& step : steps) {
      assert(step.rel_time < duration);
      if (step.type != Step::Type::STATE)
        continue;
      int val = step.new_state ? 1 : 0;
      switch (step.entity) {
        case Step::Entity::SND:
          ctx.sender_profile << ctx.start_time + step.rel_time << " " << val << std::endl;
          break;
        case Step::Entity::RCV:
          ctx.receiver_profile << ctx.start_time + step.rel_time << " " << val << std::endl;
          break;
        case Step::Entity::LNK:
          ctx.link_profile << ctx.start_time + step.rel_time << " " << val << std::endl;
          break;
        default:
          DIE_IMPOSSIBLE;
      }
    }
    Scenario scen{type, ctx.start_time, duration, sender_expected, receiver_expected, steps, ctx.index};
    ctx.scenarios.push_back(scen);
    ctx.active++;
  }
  ctx.index++;
  ctx.start_time += duration;
}

/*************************************************************************************************/

// A bunch of dirty macros to help readability (supposedly)
#define MAKE_SCENARIO(type, duration, snd_expected, rcv_expected, steps...)                                            \
  prepareScenario(ctx, CommType::type, duration, Action::snd_expected, Action::rcv_expected, {steps})

// Link
static Step loff(double rel_time)
{
  return {rel_time, Step::Type::STATE, Step::Entity::LNK, Action::END, false};
}
static Step lon(double rel_time)
{
  return {rel_time, Step::Type::STATE, Step::Entity::LNK, Action::END, true};
}
// Sender
static Step soff(double rel_time)
{
  return {rel_time, Step::Type::STATE, Step::Entity::SND, Action::END, false};
}
static Step son(double rel_time)
{
  return {rel_time, Step::Type::STATE, Step::Entity::SND, Action::END, true};
}
static Step sput(double rel_time)
{
  return {rel_time, Step::Type::ACTION, Step::Entity::SND, Action::PUT, false};
}
static Step swait(double rel_time)
{
  return {rel_time, Step::Type::ACTION, Step::Entity::SND, Action::WAIT, false};
}
// Receiver
static Step roff(double rel_time)
{
  return {rel_time, Step::Type::STATE, Step::Entity::RCV, Action::END, false};
}
static Step ron(double rel_time)
{
  return {rel_time, Step::Type::STATE, Step::Entity::RCV, Action::END, true};
}
static Step rget(double rel_time)
{
  return {rel_time, Step::Type::ACTION, Step::Entity::RCV, Action::GET, false};
}
static Step rwait(double rel_time)
{
  return {rel_time, Step::Type::ACTION, Step::Entity::RCV, Action::WAIT, false};
}

static double build_scenarios(ScenarioContext& ctx)
{
  ctx.start_time = 0;
  ctx.index      = 0;
  ctx.active     = 0;

  // EAGER SYNC use cases
  // All good
  MAKE_SCENARIO(EAGER_SYNC, 1, END, END, sput(.2), rget(.4));
  MAKE_SCENARIO(EAGER_SYNC, 1, END, END, rget(.2), sput(.4));
  // Receiver off
  MAKE_SCENARIO(EAGER_SYNC, 2, PUT, DIE, roff(.1), sput(.2), ron(1));
  MAKE_SCENARIO(EAGER_SYNC, 2, PUT, DIE, sput(.2), roff(.3), ron(1));
  MAKE_SCENARIO(EAGER_SYNC, 2, PUT, DIE, sput(.2), rget(.4), roff(.5), ron(1));
  MAKE_SCENARIO(EAGER_SYNC, 2, PUT, DIE, rget(.2), sput(.4), roff(.5), ron(1));
  // Sender off
  MAKE_SCENARIO(EAGER_SYNC, 2, DIE, GET, sput(.2), soff(.3), rget(.4), son(1));
  MAKE_SCENARIO(EAGER_SYNC, 2, DIE, GET, sput(.2), rget(.4), soff(.5), son(1));
  // Link off
  MAKE_SCENARIO(EAGER_SYNC, 2, PUT, GET, loff(.1), sput(.2), rget(.4), lon(1));
  MAKE_SCENARIO(EAGER_SYNC, 2, PUT, GET, sput(.2), loff(.3), rget(.4), lon(1));
  MAKE_SCENARIO(EAGER_SYNC, 2, PUT, GET, sput(.2), rget(.4), loff(.5), lon(1));
  MAKE_SCENARIO(EAGER_SYNC, 2, PUT, GET, loff(.1), rget(.2), sput(.4), lon(1));
  MAKE_SCENARIO(EAGER_SYNC, 2, PUT, GET, rget(.2), loff(.3), sput(.4), lon(1));
  MAKE_SCENARIO(EAGER_SYNC, 2, PUT, GET, rget(.2), sput(.4), loff(.5), lon(1));

  // EAGER ASYNC use cases
  // All good
  MAKE_SCENARIO(EAGER_ASYNC, 2, END, END, sput(.2), swait(.4), rget(.6), rwait(.8));
  MAKE_SCENARIO(EAGER_ASYNC, 2, END, END, sput(.2), rget(.4), swait(.6), rwait(.8));
  MAKE_SCENARIO(EAGER_ASYNC, 2, END, END, sput(.2), rget(.4), rwait(.6), swait(.8));
  MAKE_SCENARIO(EAGER_ASYNC, 2, END, END, rget(.2), sput(.4), swait(.6), rwait(.8));
  MAKE_SCENARIO(EAGER_ASYNC, 2, END, END, rget(.2), sput(.4), rwait(.6), swait(.8));
  MAKE_SCENARIO(EAGER_ASYNC, 2, END, END, rget(.2), rwait(.4), sput(.6), swait(.8));
  // Receiver off
  MAKE_SCENARIO(EAGER_ASYNC, 2, PUT, DIE, roff(.1), sput(.2), swait(.4), ron(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, DIE, sput(.2), roff(.3), swait(.4), ron(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, PUT, DIE, rget(.2), roff(.3), sput(.4), swait(.6), ron(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, DIE, sput(.2), swait(.4), roff(.5), ron(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, DIE, sput(.2), rget(.4), roff(.5), swait(.6), ron(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, DIE, rget(.2), sput(.4), roff(.5), swait(.6), ron(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, PUT, DIE, rget(.2), rwait(.4), roff(.5), sput(.6), swait(.8), ron(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, DIE, sput(.2), swait(.4), rget(.6), roff(.7), ron(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, DIE, sput(.2), rget(.4), swait(.6), roff(.7), ron(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, DIE, sput(.2), rget(.4), rwait(.6), roff(.7), swait(.8), ron(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, DIE, rget(.2), sput(.4), swait(.6), roff(.7), ron(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, DIE, rget(.2), sput(.4), rwait(.6), roff(.7), swait(.8), ron(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, DIE, rget(.2), rwait(.4), sput(.6), roff(.7), swait(.8), ron(1));
  // Sender off (only cases where sender did put, because otherwise receiver cannot find out there was a fault)
  MAKE_SCENARIO(EAGER_ASYNC, 2, DIE, GET, sput(.2), soff(.3), rget(.4), rwait(.6), son(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, DIE, WAIT, rget(.2), sput(.4), soff(.5), rwait(.6), son(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, DIE, WAIT, sput(.2), rget(.4), soff(.5), rwait(.6), son(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, DIE, GET, sput(.2), swait(.4), soff(.5), rget(.6), rwait(.8), son(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, DIE, WAIT, rget(.2), rwait(.4), sput(.6), soff(.7), son(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, DIE, WAIT, rget(.2), sput(.4), rwait(.6), soff(.7), son(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, DIE, WAIT, rget(.2), sput(.4), swait(.6), soff(.7), rwait(.8), son(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, DIE, WAIT, sput(.2), rget(.4), rwait(.6), soff(.7), son(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, DIE, WAIT, sput(.2), rget(.4), swait(.6), soff(.7), rwait(.8), son(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, DIE, WAIT, sput(.2), swait(.4), rget(.6), soff(.7), rwait(.8), son(1));
  // Link off
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, loff(.1), sput(.2), swait(.4), rget(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, loff(.1), sput(.2), rget(.4), swait(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, loff(.1), sput(.2), rget(.4), rwait(.6), swait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, loff(.1), rget(.2), sput(.4), swait(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, loff(.1), rget(.2), sput(.4), rwait(.6), swait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, loff(.1), rget(.2), rwait(.4), sput(.6), swait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, sput(.2), loff(.3), swait(.4), rget(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, sput(.2), loff(.3), rget(.4), swait(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, sput(.2), loff(.3), rget(.4), rwait(.6), swait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, rget(.2), loff(.3), sput(.4), swait(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, rget(.2), loff(.3), sput(.4), rwait(.6), swait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, rget(.2), loff(.3), rwait(.4), sput(.6), swait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, sput(.2), swait(.4), loff(.5), rget(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, sput(.2), rget(.4), loff(.5), swait(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, sput(.2), rget(.4), loff(.5), rwait(.6), swait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, rget(.2), sput(.4), loff(.5), swait(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, rget(.2), sput(.4), loff(.5), rwait(.6), swait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, rget(.2), rwait(.4), loff(.5), sput(.6), swait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, sput(.2), swait(.4), rget(.6), loff(.7), rwait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, sput(.2), rget(.4), swait(.6), loff(.7), rwait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, sput(.2), rget(.4), rwait(.6), loff(.7), swait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, rget(.2), sput(.4), swait(.6), loff(.7), rwait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, rget(.2), sput(.4), rwait(.6), loff(.7), swait(.8), lon(1));
  MAKE_SCENARIO(EAGER_ASYNC, 2, WAIT, WAIT, rget(.2), rwait(.4), sput(.6), loff(.7), swait(.8), lon(1));

  // RDV SYNC use cases
  // All good
  MAKE_SCENARIO(RDV_SYNC, 1, END, END, sput(.2), rget(.4));
  MAKE_SCENARIO(RDV_SYNC, 1, END, END, rget(.2), sput(.4));
  // Receiver off
  MAKE_SCENARIO(RDV_SYNC, 2, PUT, DIE, roff(.1), sput(.2), ron(1));
  MAKE_SCENARIO(RDV_SYNC, 2, PUT, DIE, sput(.2), roff(.3),
                ron(1)); // Fails because put comm cancellation does not trigger sender exception
  MAKE_SCENARIO(RDV_SYNC, 2, PUT, DIE, sput(.2), rget(.4), roff(.5), ron(1));
  MAKE_SCENARIO(RDV_SYNC, 2, PUT, DIE, rget(.2), sput(.4), roff(.5), ron(1));
  // Sender off
  MAKE_SCENARIO(RDV_SYNC, 2, DIE, GET, sput(.2), rget(.4), soff(.5), son(1));
  // Link off
  MAKE_SCENARIO(RDV_SYNC, 2, PUT, GET, loff(.1), sput(.2), rget(.4), lon(1));
  MAKE_SCENARIO(RDV_SYNC, 2, PUT, GET, sput(.2), loff(.3), rget(.4), lon(1));
  MAKE_SCENARIO(RDV_SYNC, 2, PUT, GET, sput(.2), rget(.4), loff(.5), lon(1));
  MAKE_SCENARIO(RDV_SYNC, 2, PUT, GET, loff(.1), rget(.2), sput(.4), lon(1));
  MAKE_SCENARIO(RDV_SYNC, 2, PUT, GET, rget(.2), loff(.3), sput(.4), lon(1));
  MAKE_SCENARIO(RDV_SYNC, 2, PUT, GET, rget(.2), sput(.4), loff(.5), lon(1));

  // RDV ASYNC use cases
  // All good
  MAKE_SCENARIO(RDV_ASYNC, 2, END, END, sput(.2), swait(.4), rget(.6), rwait(.8));
  MAKE_SCENARIO(RDV_ASYNC, 2, END, END, sput(.2), rget(.4), swait(.6), rwait(.8));
  MAKE_SCENARIO(RDV_ASYNC, 2, END, END, sput(.2), rget(.4), rwait(.6), swait(.8));
  MAKE_SCENARIO(RDV_ASYNC, 2, END, END, rget(.2), sput(.4), swait(.6), rwait(.8));
  MAKE_SCENARIO(RDV_ASYNC, 2, END, END, rget(.2), sput(.4), rwait(.6), swait(.8));
  MAKE_SCENARIO(RDV_ASYNC, 2, END, END, rget(.2), rwait(.4), sput(.6), swait(.8));
  // Receiver off
  MAKE_SCENARIO(RDV_ASYNC, 2, PUT, DIE, roff(.1), sput(.2), swait(.4), ron(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, DIE, sput(.2), roff(.3), swait(.4), ron(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, PUT, DIE, rget(.2), roff(.3), sput(.4), swait(.6), ron(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, DIE, sput(.2), swait(.4), roff(.5),
                ron(1)); // Fails because put comm cancellation does not trigger sender exception
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, DIE, sput(.2), rget(.4), roff(.5), swait(.6), ron(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, DIE, rget(.2), sput(.4), roff(.5), swait(.6), ron(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, PUT, DIE, rget(.2), rwait(.4), roff(.5), sput(.6), swait(.8), ron(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, DIE, sput(.2), swait(.4), rget(.6), roff(.7), ron(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, DIE, sput(.2), rget(.4), swait(.6), roff(.7), ron(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, DIE, sput(.2), rget(.4), rwait(.6), roff(.7), swait(.8), ron(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, DIE, rget(.2), sput(.4), swait(.6), roff(.7), ron(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, DIE, rget(.2), sput(.4), rwait(.6), roff(.7), swait(.8), ron(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, DIE, rget(.2), rwait(.4), sput(.6), roff(.7), swait(.8), ron(1));
  // Sender off (only cases where sender did put, because otherwise receiver cannot find out there was a fault)
  MAKE_SCENARIO(RDV_ASYNC, 2, DIE, GET, sput(.2), soff(.3), rget(.4), rwait(.6), son(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, DIE, WAIT, rget(.2), sput(.4), soff(.5), rwait(.6), son(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, DIE, WAIT, sput(.2), rget(.4), soff(.5), rwait(.6), son(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, DIE, GET, sput(.2), swait(.4), soff(.5), rget(.6), rwait(.8), son(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, DIE, WAIT, rget(.2), rwait(.4), sput(.6), soff(.7), son(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, DIE, WAIT, rget(.2), sput(.4), rwait(.6), soff(.7), son(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, DIE, WAIT, rget(.2), sput(.4), swait(.6), soff(.7), rwait(.8), son(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, DIE, WAIT, sput(.2), rget(.4), rwait(.6), soff(.7), son(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, DIE, WAIT, sput(.2), rget(.4), swait(.6), soff(.7), rwait(.8), son(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, DIE, WAIT, sput(.2), swait(.4), rget(.6), soff(.7), rwait(.8), son(1));
  // Link off
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, loff(.1), sput(.2), swait(.4), rget(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, loff(.1), sput(.2), rget(.4), swait(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, loff(.1), sput(.2), rget(.4), rwait(.6), swait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, loff(.1), rget(.2), sput(.4), swait(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, loff(.1), rget(.2), sput(.4), rwait(.6), swait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, loff(.1), rget(.2), rwait(.4), sput(.6), swait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, sput(.2), loff(.3), swait(.4), rget(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, sput(.2), loff(.3), rget(.4), swait(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, sput(.2), loff(.3), rget(.4), rwait(.6), swait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, rget(.2), loff(.3), sput(.4), swait(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, rget(.2), loff(.3), sput(.4), rwait(.6), swait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, rget(.2), loff(.3), rwait(.4), sput(.6), swait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, sput(.2), swait(.4), loff(.5), rget(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, sput(.2), rget(.4), loff(.5), swait(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, sput(.2), rget(.4), loff(.5), rwait(.6), swait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, rget(.2), sput(.4), loff(.5), swait(.6), rwait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, rget(.2), sput(.4), loff(.5), rwait(.6), swait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, rget(.2), rwait(.4), loff(.5), sput(.6), swait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, sput(.2), swait(.4), rget(.6), loff(.7), rwait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, sput(.2), rget(.4), swait(.6), loff(.7), rwait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, sput(.2), rget(.4), rwait(.6), loff(.7), swait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, rget(.2), sput(.4), swait(.6), loff(.7), rwait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, rget(.2), sput(.4), rwait(.6), loff(.7), swait(.8), lon(1));
  MAKE_SCENARIO(RDV_ASYNC, 2, WAIT, WAIT, rget(.2), rwait(.4), sput(.6), loff(.7), swait(.8), lon(1));

  // ONESIDE SYNC use cases
  // All good
  MAKE_SCENARIO(ONESIDE_SYNC, 1, END, END, sput(.2));
  // Receiver off
  MAKE_SCENARIO(ONESIDE_SYNC, 2, PUT, DIE, roff(.1), sput(.2), ron(1));
  MAKE_SCENARIO(ONESIDE_SYNC, 2, PUT, DIE, sput(.2), roff(.3), ron(1));
  // Sender off
  MAKE_SCENARIO(ONESIDE_SYNC, 2, DIE, END, sput(.2), soff(.3), son(1));
  // Link off
  MAKE_SCENARIO(ONESIDE_SYNC, 2, PUT, END, loff(.1), sput(.2), lon(1));
  MAKE_SCENARIO(ONESIDE_SYNC, 2, PUT, END, sput(.2), loff(.3), lon(1));

  // ONESIDE ASYNC use cases
  // All good
  MAKE_SCENARIO(ONESIDE_ASYNC, 2, END, END, sput(.2), swait(.4));
  // Receiver off
  MAKE_SCENARIO(ONESIDE_ASYNC, 2, PUT, DIE, roff(.1), sput(.2), swait(.4), ron(1));
  MAKE_SCENARIO(ONESIDE_ASYNC, 2, WAIT, DIE, sput(.2), roff(.3), swait(.4), ron(1));
  MAKE_SCENARIO(ONESIDE_ASYNC, 2, WAIT, DIE, sput(.2), swait(.4), roff(.5), ron(1));
  // Sender off
  MAKE_SCENARIO(ONESIDE_ASYNC, 2, DIE, END, sput(.2), soff(.3), son(1));
  // Link off
  MAKE_SCENARIO(ONESIDE_ASYNC, 2, WAIT, END, loff(.1), sput(.2), swait(.4), lon(1));
  MAKE_SCENARIO(ONESIDE_ASYNC, 2, WAIT, END, sput(.2), loff(.3), swait(.4), lon(1));
  MAKE_SCENARIO(ONESIDE_ASYNC, 2, WAIT, END, sput(.2), swait(.4), loff(.5), lon(1));

  XBT_INFO("Will execute %i active scenarios out of %i.", ctx.active, ctx.index);
  return ctx.start_time + 1;
}
