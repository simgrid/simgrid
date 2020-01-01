/* Copyright (c) 2016-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef S4U_CHORD_HPP
#define S4U_CHORD_HPP
#include "simgrid/s4u.hpp"
#include <string>
#include <xbt/random.hpp>
#include <xbt/str.h>

constexpr double MAX_SIMULATION_TIME              = 1000;
constexpr double PERIODIC_STABILIZE_DELAY         = 20;
constexpr double PERIODIC_FIX_FINGERS_DELAY       = 120;
constexpr double PERIODIC_CHECK_PREDECESSOR_DELAY = 120;
constexpr double PERIODIC_LOOKUP_DELAY            = 10;
constexpr double SLEEP_DELAY                      = 4.9999;

extern int nb_bits;
extern int nb_keys;
extern int timeout;

/* Types of tasks exchanged between nodes. */
enum e_message_type_t {
  FIND_SUCCESSOR,
  FIND_SUCCESSOR_ANSWER,
  GET_PREDECESSOR,
  GET_PREDECESSOR_ANSWER,
  NOTIFY,
  SUCCESSOR_LEAVING,
  PREDECESSOR_LEAVING,
  PREDECESSOR_ALIVE,
  PREDECESSOR_ALIVE_ANSWER
};

class ChordMessage {
public:
  e_message_type_t type;              // type of message
  std::string issuer_host_name     = simgrid::s4u::this_actor::get_host()->get_name(); // used for logging
  int request_id     = -1;            // id (used by some types of messages)
  int request_finger = 1;             // finger parameter (used by some types of messages)
  int answer_id      = -1;            // answer (used by some types of messages)
  simgrid::s4u::Mailbox* answer_to = nullptr;       // mailbox to send an answer to (if any)

  explicit ChordMessage(e_message_type_t type) : type(type) {}

  static void destroy(void* message);
};

class Node {
  int known_id_      = -1;
  double start_time_ = -1;
  double deadline_   = -1;
  bool joined        = false;
  int id_;                           // my id
  int pred_id_ = -1;                 // predecessor id
  simgrid::s4u::Mailbox* mailbox_;   // my mailbox
  std::vector<int> fingers_;         // finger table,(fingers[0] is my successor)
  int next_finger_to_fix;            // index of the next finger to fix in fix_fingers()

public:
  explicit Node(std::vector<std::string> args);
  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;
  void join(int known_id);
  void leave();
  void notifyAndQuit();

  void randomLookup();
  void setFinger(int finger_index, int id);
  void fixFingers();
  void printFingerTable();

  void setPredecessor(int predecessor_id);
  void checkPredecessor();
  int remoteGetPredecessor(int ask_to);
  int closestPrecedingFinger(int id);
  int findSuccessor(int id);
  int remoteFindSuccessor(int ask_to, int id);

  void notify(int predecessor_candidate_id);
  void remoteNotify(int notify_id, int predecessor_candidate_id);
  void stabilize();
  void handleMessage(ChordMessage* message);

  void operator()()
  {
    simgrid::s4u::this_actor::sleep_for(start_time_);
    if (known_id_ == -1) {
      setPredecessor(-1); // -1 means that I have no predecessor
      printFingerTable();
      joined = true;
    } else {
      join(known_id_);
    }

    if (not joined)
      return;
    void* data                         = nullptr;
    double now                         = simgrid::s4u::Engine::get_clock();
    double next_stabilize_date         = start_time_ + PERIODIC_STABILIZE_DELAY;
    double next_fix_fingers_date       = start_time_ + PERIODIC_FIX_FINGERS_DELAY;
    double next_check_predecessor_date = start_time_ + PERIODIC_CHECK_PREDECESSOR_DELAY;
    double next_lookup_date            = start_time_ + PERIODIC_LOOKUP_DELAY;
    simgrid::s4u::CommPtr comm_receive = nullptr;
    while ((now < (start_time_ + deadline_)) && now < MAX_SIMULATION_TIME) {
      if (comm_receive == nullptr)
        comm_receive = mailbox_->get_async(&data);
      while ((now < (start_time_ + deadline_)) && now < MAX_SIMULATION_TIME && not comm_receive->test()) {
        // no task was received: make some periodic calls
        if (now >= next_stabilize_date) {
          stabilize();
          next_stabilize_date = simgrid::s4u::Engine::get_clock() + PERIODIC_STABILIZE_DELAY;
        } else if (now >= next_fix_fingers_date) {
          fixFingers();
          next_fix_fingers_date = simgrid::s4u::Engine::get_clock() + PERIODIC_FIX_FINGERS_DELAY;
        } else if (now >= next_check_predecessor_date) {
          checkPredecessor();
          next_check_predecessor_date = simgrid::s4u::Engine::get_clock() + PERIODIC_CHECK_PREDECESSOR_DELAY;
        } else if (now >= next_lookup_date) {
          randomLookup();
          next_lookup_date = simgrid::s4u::Engine::get_clock() + PERIODIC_LOOKUP_DELAY;
        } else {
          // nothing to do: sleep for a while
          simgrid::s4u::this_actor::sleep_for(SLEEP_DELAY);
        }
        now = simgrid::s4u::Engine::get_clock();
      }

      if (data != nullptr) {
        ChordMessage* message = static_cast<ChordMessage*>(data);
        handleMessage(message);
        comm_receive = nullptr;
        data         = nullptr;
      }
      now = simgrid::s4u::Engine::get_clock();
    }
    if (comm_receive != nullptr) {
      if (comm_receive->test())
        delete static_cast<ChordMessage*>(data);
      else
        comm_receive->cancel();
    }
    // leave the ring
    leave();
  }
};

#endif
