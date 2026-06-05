/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/Transition.hpp"
#include "src/mc/remote/Channel.hpp"
#include "xbt/asserts.h"
#include "xbt/string.hpp"
#include <simgrid/config.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <utility>

#if SIMGRID_HAVE_MC
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/transition/TransitionActor.hpp"
#include "src/mc/transition/TransitionAny.hpp"
#include "src/mc/transition/TransitionComm.hpp"
#include "src/mc/transition/TransitionObjectAccess.hpp"
#include "src/mc/transition/TransitionRandom.hpp"
#include "src/mc/transition/TransitionSynchro.hpp"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_transition, mc, "Logging specific to MC transitions");

namespace simgrid::mc {
std::atomic_ulong Transition::executed_transitions_ = 0;
std::atomic_ulong Transition::replayed_transitions_ = 0;
const std::string Transition::empty_string          = "";

// Do not move this to the header, to ensure that we have a vtable for Transition
Transition::~Transition() = default;

std::string Transition::to_string(bool) const
{
  return "";
}
std::string Transition::dot_string() const
{
  static constexpr std::array<const char*, 13> colors{{"blue", "red", "green3", "goldenrod", "brown", "purple",
                                                       "magenta", "turquoise4", "gray25", "forestgreen", "hotpink",
                                                       "lightblue", "tan"}};
  const char* color = colors[(aid_.value() - 1) % colors.size()];

  return xbt::string_printf("label = \"[(%d)] %s\", color = %s, fontcolor = %s", aid_.c_val(),
                            Transition::to_c_str(type_), color, color);
}
void Transition::replay(RemoteApp& app) const
{
  replayed_transitions_++;
#if SIMGRID_HAVE_MC
  app.handle_simcall(aid_, times_considered_, false);
  app.wait_for_requests();
#endif
}

void Transition::deserialize_memory_tracker(mc::Channel& channel)
{
  get_memory_trace().deserialize(channel);

  XBT_DEBUG("Created a memory access tracker for the new transition");
}

Transition* deserialize_transition(Aid issuer, int times_considered, mc::Channel& channel)
{
#if SIMGRID_HAVE_MC
  mc::Transition::Type simcall = channel.unpack<mc::Transition::Type>();

  XBT_DEBUG("Deserializing a %s transition", mc::Transition::to_c_str(simcall));
  switch (simcall) {
    case Transition::Type::BARRIER_ASYNC_LOCK:
    case Transition::Type::BARRIER_WAIT:
      return new BarrierTransition(issuer, times_considered, simcall, channel);

    case Transition::Type::COMM_ASYNC_RECV:
      return new CommRecvTransition(issuer, times_considered, channel);
    case Transition::Type::COMM_ASYNC_SEND:
      return new CommSendTransition(issuer, times_considered, channel);
    case Transition::Type::COMM_IPROBE:
      return new CommIprobeTransition(issuer, times_considered, channel);
    case Transition::Type::COMM_TEST:
      return new CommTestTransition(issuer, times_considered, channel);
    case Transition::Type::COMM_WAIT:
      return new CommWaitTransition(issuer, times_considered, channel);

    case Transition::Type::TESTANY:
      return new TestAnyTransition(issuer, times_considered, channel);
    case Transition::Type::WAITANY:
      return new WaitAnyTransition(issuer, times_considered, channel);

    case Transition::Type::RANDOM:
      return new RandomTransition(issuer, times_considered, channel);

    case Transition::Type::MUTEX_TRYLOCK:
    case Transition::Type::MUTEX_ASYNC_LOCK:
    case Transition::Type::MUTEX_TEST:
    case Transition::Type::MUTEX_WAIT:
    case Transition::Type::MUTEX_UNLOCK:
      return new MutexTransition(issuer, times_considered, simcall, channel);

    case Transition::Type::SEM_ASYNC_LOCK:
    case Transition::Type::SEM_UNLOCK:
    case Transition::Type::SEM_WAIT:
      return new SemaphoreTransition(issuer, times_considered, simcall, channel);

    case Transition::Type::CONDVAR_ASYNC_LOCK:
    case Transition::Type::CONDVAR_BROADCAST:
    case Transition::Type::CONDVAR_SIGNAL:
    case Transition::Type::CONDVAR_WAIT:
      return new CondvarTransition(issuer, times_considered, simcall, channel);

    case Transition::Type::ACTOR_CREATE:
      return new ActorCreateTransition(issuer, times_considered, channel);
    case Transition::Type::ACTOR_EXIT:
      return new ActorExitTransition(issuer, times_considered, channel);
    case Transition::Type::ACTOR_JOIN:
      return new ActorJoinTransition(issuer, times_considered, channel);
    case Transition::Type::ACTOR_SLEEP:
      return new ActorSleepTransition(issuer, times_considered, channel);

    case Transition::Type::OBJECT_ACCESS:
      return new ObjectAccessTransition(issuer, times_considered, channel);

    case Transition::Type::UNKNOWN:
      return new Transition(Transition::Type::UNKNOWN, issuer, times_considered);

    default:
      break;
  }
  xbt_die("Invalid transition type %d received. Did you implement a new observer in the app without implementing the "
          "corresponding transition in the checker?",
          (int)simcall);
#else
  xbt_die("Deserializing transitions is only interesting in MC mode.");
#endif
}

#if SIMGRID_HAVE_MC
/* *****************************************************************************************************************************
 *                                                   Dependency Analysis
 * -----------------------------------------------------------------------------------------------------------------------------
 * The code in charge of computing the dependencies is carefuly optimized because it's on the critical path of the
 * verification. Each time we reach the end of an Execution, we need to compute the dependency of each transition with
 * all the transitions preceeding it, making this part O(n²).  This is computed by Transition::dispatch_depends()
 *
 * This function uses a look-up table to find the answer as quickly as possible. The table gives the following mapping:
 *    {Transition::Type,Transition::Type} |-> DependencyAction
 *
 * Many components of the table map to DependencyAction::ALWAYS_INDEP (meaning that these transitions are independent)
 * or DependencyAction::ALWAYS_DEP (for pairs of transition that are always independent). If the dependency is a
 * condition, a specific DependencyAction is used. Then, after the table look-up, this value is used in a big switch
 * that casts the transitions and computes the condition according. At the end of the day,
 * Transition::dispatch_depends() is very short and efficient. After some filtering, we use the table (constant-time)
 * and then jump to the corresponding case of a big switch. Most switch cases are very small and simple, since most
 * conditional elements were computed by the look-up table directly.
 *
 * The challenge in this code is to fill the look-up table (LUT) in a readable manner. This table is crucial to the
 * validity of the whole verification process. Each cell was proven manually in The Anh's or Mathieu's PhD, we want it
 * to be readable to allow further verifications.
 *
 * For sake of readability, the values in the LUT have explicit names as EVAL_T1_ACTOR_JOIN meaning that T1 is an
 * ACTOR_JOIN.
 *
 * We also provide several macros to help filling this table:
 *   - rule(t1, t2, DependencyAction): maps {t1,t2} to that DependencyAction.
 *   - rule_all(t1, DependencyAction): maps {t1,_} to that DependencyAction, for any second type
 *   - fill_group(group, DependencyAction): maps any combination within the group (specified as an interval of types)
 *   - cross_indep(group1, group2): specifies that any element of group1 is INDEP with any element of group2
 *
 * Don't worry, the code is paranoid. A compile-time error is raised on error to help you filling this table correctly.
 * You are informed if you overwrite a rule by error. If you really want to overwrite it, add 'true' as extra parameter
 * to the macro you use (all macros have an optional parameter 'overwrite' set to false by default. Missing rules are
 * also reported. Additionaly, rule(t1, t2) reports an error if t1 > t2, so that the casts are easier to write in the
 * switch cases (your left transition is t1) despite the fact that the code symmetrizes the relation.
 *
 * When an error is reported, compile with clang which makes more informative messages, and search for "in call" string.
 * You will get something similar to the following: >>in call to 'this->missing_lut_rule(15, 17)'<<
 *
 * This by example means that you forgot the rule between the type 15 and the type 17. Check the enum to find the
 * involved types (by counting the elements). This may sound somewhat scary, but I think that adding a new rule is
 * actually very doable.
 */

// Enumeration of all discrete semantic rules for testing transition dependency.
enum class DependencyAction : uint8_t {
  // Universal rules
  ALWAYS_INDEP,
  ALWAYS_DEP,

  // Boundary constraints & Fallbacks
  PANIC_UNWRAPPED_ANY,  // TESTANY/WAITANY are supposed to be unwrapped before reading the LUT
  PANIC_NOMC,           // NOMC transitions must not be evaluated here
  EVAL_DYNAMIC_UNKNOWN, // Defer to the virtual depends() method for mock testing and critical transition lookup

  // Encapsulated state evaluations because these depend on private fields (FIXME)
  EVAL_BARRIER_DEPENDS,
  EVAL_OBJECT_DEPENDS,

  // Actor rules
  EVAL_T1_ACTOR_JOIN,
  EVAL_T2_ACTOR_JOIN,
  EVAL_BOTH_ACTOR_JOIN,
  EVAL_T1_ACTOR_CREATE,
  EVAL_T2_ACTOR_CREATE,
  EVAL_BOTH_ACTOR_CREATE,

  // Synchronization rules
  EVAL_MUTEX_ID,
  EVAL_SEM_ID,
  EVAL_CONDVAR_WAKEUP,
  EVAL_CONDVAR_WAIT_MUTEX,
  EVAL_CONDVAR_MUTEX_WAIT,
  EVAL_CONDVAR_MUTEX_UNLOCK,

  // Communication rules. Many combinations need a specific treatment
  EVAL_COMM_RECV_RECV,
  EVAL_COMM_RECV_IPROBE,
  EVAL_COMM_RECV_TEST,
  EVAL_COMM_RECV_WAIT,
  EVAL_COMM_SEND_SEND,
  EVAL_COMM_SEND_IPROBE,
  EVAL_COMM_SEND_TEST,
  EVAL_COMM_SEND_WAIT,
  EVAL_COMM_IPROBE_MBOX,
  EVAL_COMM_WAIT_WAIT,
  EVAL_COMM_TEST_WAIT
};

using TypeGroup = std::pair<Transition::Type, Transition::Type>;

/**
 * Builder class executed at compile-time (consteval) to compute the 2D LUT.
 * Enforces definition of all pairwise transition combinations to guarantee formal safety.
 */
template <size_t N> class DependencyTableBuilder {
  std::array<std::array<DependencyAction, N>, N> table_{};
  std::array<std::array<bool, N>, N> init_{};

public:
  constexpr DependencyTableBuilder()
  {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        init_[i][j] = false;
      }
    }
  }
  constexpr void overwrite_lut_rule([[maybe_unused]] size_t i, [[maybe_unused]] size_t j) const
  {
    throw "A pairwise transition dependency rule is overwritten in the LUT! Clang shows the involved transitions in "
          "the call stack as parameters to this->overwrite_lut_rule()";
  }
  constexpr void inverted_lut_rule([[maybe_unused]] size_t i, [[maybe_unused]] size_t j) const
  {
    throw "A pairwise transition dependency rule is defined in reverse order in the LUT! Clang shows the involved "
          "transitions in the call stack as parameters to this->inverted_lut_rule()";
  }

  // Register a symmetric rule between two types with optional overwrite assertion protection
  constexpr DependencyTableBuilder& rule(Transition::Type t1, Transition::Type t2, DependencyAction action,
                                         bool overwrite = false)
  {
    size_t i = static_cast<size_t>(t1);
    size_t j = static_cast<size_t>(t2);

    if (j < i) // This is enforced to not introduce falltraps in the switch cases: t1 and t2 are who you think they are
      inverted_lut_rule(i, j);

    if (not overwrite && init_[i][j])
      overwrite_lut_rule(i, j);

    table_[i][j] = action;
    table_[j][i] = action;
    init_[i][j]  = true;
    init_[j][i]  = true;
    return *this;
  }

  // Define a rule for a specific type against ALL other transition types
  constexpr DependencyTableBuilder& rule_all(Transition::Type t, DependencyAction action, bool overwrite = false)
  {
    uint8_t i = static_cast<uint8_t>(t);
    for (size_t j = 0; j < N; j++) {
      if (i < j)
        rule(t, static_cast<Transition::Type>(j), action, overwrite);
      else
        rule(static_cast<Transition::Type>(j), t, action, overwrite);
    }
    return *this;
  }

  // Inject asymmetric actions for Actor Join blanket row/columns
  constexpr DependencyTableBuilder& rule_all_actor_join(bool overwrite = false)
  {
    size_t join_idx = static_cast<size_t>(Transition::Type::ACTOR_JOIN);
    for (size_t i = 0; i < N; ++i) {
      Transition::Type t_i = static_cast<Transition::Type>(i);
      if (i < join_idx) {
        rule(t_i, Transition::Type::ACTOR_JOIN, DependencyAction::EVAL_T2_ACTOR_JOIN, overwrite);
      } else if (i > join_idx) {
        rule(Transition::Type::ACTOR_JOIN, t_i, DependencyAction::EVAL_T1_ACTOR_JOIN, overwrite);
      } else {
        rule(Transition::Type::ACTOR_JOIN, Transition::Type::ACTOR_JOIN, DependencyAction::EVAL_BOTH_ACTOR_JOIN,
             overwrite);
      }
    }
    return *this;
  }

  // Inject asymmetric actions for Actor Create blanket row/columns
  constexpr DependencyTableBuilder& rule_all_actor_create(bool overwrite = false)
  {
    size_t create_idx = static_cast<size_t>(Transition::Type::ACTOR_CREATE);
    for (size_t i = 0; i < N; ++i) {
      Transition::Type t_i = static_cast<Transition::Type>(i);
      if (i < create_idx) {
        rule(t_i, Transition::Type::ACTOR_CREATE, DependencyAction::EVAL_T2_ACTOR_CREATE, overwrite);
      } else if (i > create_idx) {
        rule(Transition::Type::ACTOR_CREATE, t_i, DependencyAction::EVAL_T1_ACTOR_CREATE, overwrite);
      } else {
        rule(Transition::Type::ACTOR_CREATE, Transition::Type::ACTOR_CREATE, DependencyAction::EVAL_BOTH_ACTOR_CREATE,
             overwrite);
      }
    }
    return *this;
  }

  // Define a default rule strictly within a group block (start to end inclusive)
  constexpr DependencyTableBuilder& fill_group(TypeGroup group, DependencyAction action, bool overwrite = false)
  {
    for (size_t i = static_cast<size_t>(group.first); i <= static_cast<size_t>(group.second); ++i) {
      for (size_t j = i; j <= static_cast<size_t>(group.second); ++j) {
        rule(static_cast<Transition::Type>(i), static_cast<Transition::Type>(j), action, overwrite);
      }
    }
    return *this;
  }

  // Define cross-independence between two distinct groups
  constexpr DependencyTableBuilder& cross_indep(TypeGroup g1, TypeGroup g2, bool overwrite = false)
  {
    for (size_t i = static_cast<size_t>(g1.first); i <= static_cast<size_t>(g1.second); ++i) {
      for (size_t j = static_cast<size_t>(g2.first); j <= static_cast<size_t>(g2.second); ++j) {
        rule(static_cast<Transition::Type>(i), static_cast<Transition::Type>(j), DependencyAction::ALWAYS_INDEP,
             overwrite);
      }
    }
    return *this;
  }

  constexpr void missing_lut_rule([[maybe_unused]] size_t i, [[maybe_unused]] size_t j) const
  {
    throw "A required pairwise transition dependency rule is missing in the LUT! Clang shows the involved transitions "
          "in the call stack as parameters to this->missing_lut_rule()";
  }

  constexpr auto build() const
  {
    for (size_t i = 0; i < N; ++i)
      for (size_t j = i; j < N; ++j) // Only check the upper diagonal part
        if (not init_[i][j])
          missing_lut_rule(i, j);

    return table_;
  }
};

static constexpr size_t NUM_TYPES = static_cast<size_t>(Transition::Type::UNKNOWN) + 1;

static constexpr auto dependency_table = []() consteval {
  using Type = Transition::Type;

  // Semantic groups used to bulk-assign cross-domain independence. Only the first and last type of each group is given.
  // That's an interval, not an exhaustive list
  constexpr TypeGroup group_barrier = {Type::BARRIER_ASYNC_LOCK, Type::BARRIER_WAIT};
  constexpr TypeGroup group_comm    = {Type::COMM_ASYNC_RECV, Type::COMM_WAIT};
  constexpr TypeGroup group_mutex   = {Type::MUTEX_ASYNC_LOCK, Type::MUTEX_WAIT};
  constexpr TypeGroup group_sem     = {Type::SEM_ASYNC_LOCK, Type::SEM_WAIT};
  constexpr TypeGroup group_condvar = {Type::CONDVAR_ASYNC_LOCK, Type::CONDVAR_WAIT};

  return DependencyTableBuilder<NUM_TYPES>()

      // Cross-Group Independence (Distinct sync objects are independent)
      // -----------------------------------------------------------------------
      .cross_indep(group_barrier, group_comm)
      .cross_indep(group_barrier, group_mutex)
      .cross_indep(group_barrier, group_sem)
      .cross_indep(group_barrier, group_condvar)

      .cross_indep(group_comm, group_mutex)
      .cross_indep(group_comm, group_sem)
      .cross_indep(group_comm, group_condvar)

      .cross_indep(group_mutex, group_sem)
      .cross_indep(group_sem, group_condvar)

      // -----------------------------------------------------------------------
      // Intra-Group: Barrier
      // -----------------------------------------------------------------------
      .fill_group(group_barrier, DependencyAction::ALWAYS_INDEP)
      .rule(Type::BARRIER_ASYNC_LOCK, Type::BARRIER_WAIT, DependencyAction::EVAL_BARRIER_DEPENDS, true)

      // -----------------------------------------------------------------------
      // Intra-Group: Communication
      // -----------------------------------------------------------------------
      .fill_group(group_comm, DependencyAction::ALWAYS_INDEP)

      .rule(Type::COMM_ASYNC_RECV, Type::COMM_ASYNC_RECV, DependencyAction::EVAL_COMM_RECV_RECV, true)
      .rule(Type::COMM_ASYNC_RECV, Type::COMM_IPROBE, DependencyAction::EVAL_COMM_RECV_IPROBE, true)
      .rule(Type::COMM_ASYNC_RECV, Type::COMM_TEST, DependencyAction::EVAL_COMM_RECV_TEST, true)
      .rule(Type::COMM_ASYNC_RECV, Type::COMM_WAIT, DependencyAction::EVAL_COMM_RECV_WAIT, true)

      .rule(Type::COMM_ASYNC_SEND, Type::COMM_ASYNC_SEND, DependencyAction::EVAL_COMM_SEND_SEND, true)
      .rule(Type::COMM_ASYNC_SEND, Type::COMM_IPROBE, DependencyAction::EVAL_COMM_SEND_IPROBE, true)
      .rule(Type::COMM_ASYNC_SEND, Type::COMM_TEST, DependencyAction::EVAL_COMM_SEND_TEST, true)
      .rule(Type::COMM_ASYNC_SEND, Type::COMM_WAIT, DependencyAction::EVAL_COMM_SEND_WAIT, true)

      .rule(Type::COMM_IPROBE, Type::COMM_IPROBE, DependencyAction::EVAL_COMM_IPROBE_MBOX, true)

      .rule(Type::COMM_TEST, Type::COMM_WAIT, DependencyAction::EVAL_COMM_TEST_WAIT, true)
      .rule(Type::COMM_WAIT, Type::COMM_WAIT, DependencyAction::EVAL_COMM_WAIT_WAIT, true)

      // -----------------------------------------------------------------------
      // Intra-Group: Mutex
      // Any Mutex operation is independent from another distinct Mutex (Th 4.4.7).
      // Dependent rules are resolved by EVAL_MUTEX_ID which checks identity.
      // -----------------------------------------------------------------------
      // Th 4.4.11: LOCK indep with TEST and WAIT unconditionally
      .rule(Type::MUTEX_ASYNC_LOCK, Type::MUTEX_TEST, DependencyAction::ALWAYS_INDEP)
      .rule(Type::MUTEX_ASYNC_LOCK, Type::MUTEX_WAIT, DependencyAction::ALWAYS_INDEP)
      // Th 4.4.8: LOCK indep UNLOCK unconditionally (push_back and pop_front are independent)
      .rule(Type::MUTEX_ASYNC_LOCK, Type::MUTEX_UNLOCK, DependencyAction::ALWAYS_INDEP)
      // Th 4.4.9: Any combination of WAIT and TEST are unconditionally independent
      .rule(Type::MUTEX_TEST, Type::MUTEX_TEST, DependencyAction::ALWAYS_INDEP)
      .rule(Type::MUTEX_TEST, Type::MUTEX_WAIT, DependencyAction::ALWAYS_INDEP)
      .rule(Type::MUTEX_WAIT, Type::MUTEX_WAIT, DependencyAction::ALWAYS_INDEP)
      // Pure function rules: TEST or WAIT with TRYLOCK
      .rule(Type::MUTEX_TEST, Type::MUTEX_TRYLOCK, DependencyAction::ALWAYS_INDEP)
      .rule(Type::MUTEX_TRYLOCK, Type::MUTEX_WAIT, DependencyAction::ALWAYS_INDEP)
      // UNLOCK/UNLOCK: no more than one UNLOCK can be active at a given point
      .rule(Type::MUTEX_UNLOCK, Type::MUTEX_UNLOCK, DependencyAction::ALWAYS_INDEP)

      // Remaining combinations are dependent, IF they share the same Mutex ID
      .rule(Type::MUTEX_ASYNC_LOCK, Type::MUTEX_ASYNC_LOCK, DependencyAction::EVAL_MUTEX_ID)
      .rule(Type::MUTEX_ASYNC_LOCK, Type::MUTEX_TRYLOCK, DependencyAction::EVAL_MUTEX_ID)
      .rule(Type::MUTEX_TEST, Type::MUTEX_UNLOCK, DependencyAction::EVAL_MUTEX_ID)
      .rule(Type::MUTEX_TRYLOCK, Type::MUTEX_TRYLOCK, DependencyAction::EVAL_MUTEX_ID)
      .rule(Type::MUTEX_TRYLOCK, Type::MUTEX_UNLOCK, DependencyAction::EVAL_MUTEX_ID)
      .rule(Type::MUTEX_UNLOCK, Type::MUTEX_WAIT, DependencyAction::EVAL_MUTEX_ID)

      // -----------------------------------------------------------------------
      // Intra-Group: Semaphore
      // -----------------------------------------------------------------------
      .fill_group(group_sem, DependencyAction::EVAL_SEM_ID)
      .rule(Type::SEM_ASYNC_LOCK, Type::SEM_UNLOCK, DependencyAction::ALWAYS_INDEP, true)
      .rule(Type::SEM_ASYNC_LOCK, Type::SEM_WAIT, DependencyAction::ALWAYS_INDEP, true)
      .rule(Type::SEM_UNLOCK, Type::SEM_UNLOCK, DependencyAction::ALWAYS_INDEP, true)
      .rule(Type::SEM_WAIT, Type::SEM_WAIT, DependencyAction::ALWAYS_INDEP, true)

      // -----------------------------------------------------------------------
      // Intra-Group: Condvar
      // -----------------------------------------------------------------------
      .fill_group(group_condvar, DependencyAction::ALWAYS_INDEP)
      .rule(Type::CONDVAR_ASYNC_LOCK, Type::CONDVAR_SIGNAL, DependencyAction::EVAL_CONDVAR_WAKEUP, true)
      .rule(Type::CONDVAR_ASYNC_LOCK, Type::CONDVAR_BROADCAST, DependencyAction::EVAL_CONDVAR_WAKEUP, true)
      .rule(Type::CONDVAR_SIGNAL, Type::CONDVAR_WAIT, DependencyAction::EVAL_CONDVAR_WAKEUP, true)
      .rule(Type::CONDVAR_BROADCAST, Type::CONDVAR_WAIT, DependencyAction::EVAL_CONDVAR_WAKEUP, true)
      .rule(Type::CONDVAR_WAIT, Type::CONDVAR_WAIT, DependencyAction::EVAL_CONDVAR_WAIT_MUTEX, true)

      // -----------------------------------------------------------------------
      // Cross-Group EXCEPTIONS: Condvar behaves as Mutex locks
      // -----------------------------------------------------------------------
      .rule(Type::MUTEX_WAIT, Type::CONDVAR_ASYNC_LOCK, DependencyAction::EVAL_CONDVAR_MUTEX_UNLOCK, true)
      .rule(Type::MUTEX_UNLOCK, Type::CONDVAR_ASYNC_LOCK, DependencyAction::EVAL_CONDVAR_MUTEX_UNLOCK, true)
      .rule(Type::MUTEX_TRYLOCK, Type::CONDVAR_ASYNC_LOCK, DependencyAction::EVAL_CONDVAR_MUTEX_UNLOCK, true)

      .rule(Type::MUTEX_ASYNC_LOCK, Type::CONDVAR_WAIT, DependencyAction::EVAL_CONDVAR_MUTEX_WAIT, true)
      .rule(Type::MUTEX_TRYLOCK, Type::CONDVAR_WAIT, DependencyAction::EVAL_CONDVAR_MUTEX_WAIT, true)
      .rule(Type::MUTEX_ASYNC_LOCK, Type::CONDVAR_ASYNC_LOCK, DependencyAction::ALWAYS_INDEP)

      .rule(Type::MUTEX_ASYNC_LOCK, Type::CONDVAR_BROADCAST, DependencyAction::ALWAYS_INDEP)
      .rule(Type::MUTEX_ASYNC_LOCK, Type::CONDVAR_SIGNAL, DependencyAction::ALWAYS_INDEP)
      .rule(Type::MUTEX_TRYLOCK, Type::CONDVAR_BROADCAST, DependencyAction::ALWAYS_INDEP)
      .rule(Type::MUTEX_TRYLOCK, Type::CONDVAR_SIGNAL, DependencyAction::ALWAYS_INDEP)
      .rule(Type::MUTEX_UNLOCK, Type::CONDVAR_BROADCAST, DependencyAction::ALWAYS_INDEP)
      .rule(Type::MUTEX_UNLOCK, Type::CONDVAR_SIGNAL, DependencyAction::ALWAYS_INDEP)
      .rule(Type::MUTEX_WAIT, Type::CONDVAR_BROADCAST, DependencyAction::ALWAYS_INDEP)
      .rule(Type::MUTEX_WAIT, Type::CONDVAR_SIGNAL, DependencyAction::ALWAYS_INDEP)

      .rule(Type::MUTEX_UNLOCK, Type::CONDVAR_WAIT, DependencyAction::ALWAYS_INDEP)
      .rule(Type::MUTEX_WAIT, Type::CONDVAR_WAIT, DependencyAction::ALWAYS_INDEP)

      .rule(Type::MUTEX_TEST, Type::CONDVAR_ASYNC_LOCK, DependencyAction::EVAL_CONDVAR_MUTEX_WAIT)
      .rule(Type::MUTEX_TEST, Type::CONDVAR_BROADCAST, DependencyAction::ALWAYS_INDEP)
      .rule(Type::MUTEX_TEST, Type::CONDVAR_SIGNAL, DependencyAction::ALWAYS_INDEP)
      .rule(Type::MUTEX_TEST, Type::CONDVAR_WAIT, DependencyAction::ALWAYS_INDEP)

      // Actor Transitions
      // -----------------------------------------------------------------------
      .rule_all(Type::ACTOR_SLEEP, DependencyAction::ALWAYS_INDEP, true)
      /* ACTOR_EXIT is dependent with ACTOR_JOIN (one destroys the actor awaited by the other), but ACTOR_JOIN <
       * ACTOR_EXIT so this fact is computed by ACTOR_JOIN, so ACTOR_EXIT is noted as independent with all types after
       * it in the enumeration. */
      .rule_all(Type::ACTOR_EXIT, DependencyAction::ALWAYS_INDEP, true)

      .rule_all_actor_join(true)
      .rule_all_actor_create(true)

      // -----------------------------------------------------------------------
      // Global and Local Overrides
      // -----------------------------------------------------------------------
      .rule_all(Type::RANDOM, DependencyAction::ALWAYS_INDEP, true) // Overwrites every rules
      .rule_all(Type::OBJECT_ACCESS, DependencyAction::ALWAYS_INDEP,
                true) // Overwrites RANDOM intersection intentionally
      .rule(Type::OBJECT_ACCESS, Type::OBJECT_ACCESS, DependencyAction::EVAL_OBJECT_DEPENDS, true)

      // -----------------------------------------------------------------------
      // Safety, Fallbacks & Error States (Applied LAST via intentional force-overwrites)
      // -----------------------------------------------------------------------
      .rule_all(Type::UNKNOWN, DependencyAction::EVAL_DYNAMIC_UNKNOWN, true /*We overwrite RANDOM*/)
      .rule_all(Type::MUTEX_LOCK_NOMC, DependencyAction::PANIC_NOMC, true)
      .rule_all(Type::SEM_LOCK_NOMC, DependencyAction::PANIC_NOMC, true)
      .rule_all(Type::CONDVAR_NOMC, DependencyAction::PANIC_NOMC, true)
      .rule_all(Type::TESTANY, DependencyAction::PANIC_UNWRAPPED_ANY, true)
      .rule_all(Type::WAITANY, DependencyAction::PANIC_UNWRAPPED_ANY, true)

      .build();
}();
#endif // SIMGRID_HAVE_MC

/* This function evaluates state-space transition dependencies strictly and optimally using a compiled Look-Up Table. */
bool Transition::dispatch_depends(const Transition* other) const
{
#if SIMGRID_HAVE_MC
  const Transition* t1 = this;
  const Transition* t2 = other;

  // Actions executed by the same actor are inherently dependent (sequential thread constraint)
  if (t1->aid_ == t2->aid_)
    return true;

  // Fully unwrap wildcard ANY transitions to reveal their underlying state
  while (t1->type_ == Type::TESTANY || t1->type_ == Type::WAITANY) [[unlikely]] {
    t1 = (t1->type_ == Type::TESTANY) ? static_cast<const TestAnyTransition*>(t1)->get_current_transition()
                                      : static_cast<const WaitAnyTransition*>(t1)->get_current_transition();
  }
  while (t2->type_ == Type::TESTANY || t2->type_ == Type::WAITANY) [[unlikely]] {
    t2 = (t2->type_ == Type::TESTANY) ? static_cast<const TestAnyTransition*>(t2)->get_current_transition()
                                      : static_cast<const WaitAnyTransition*>(t2)->get_current_transition();
  }

  // Symmetrization: We enforce t1 <= t2 since the mathematical relationship of dependencies is strictly commutative.
  if (t1->type_ > t2->type_) {
    std::swap(t1, t2);
  }

  // constant-time computation of the jump to achieve to reach the right semantic evaluation rule
  DependencyAction action = dependency_table[static_cast<size_t>(t1->type_)][static_cast<size_t>(t2->type_)];

  switch (action) { // Actually jump to the right evaluation rule
    case DependencyAction::ALWAYS_INDEP:
      return false;
    case DependencyAction::ALWAYS_DEP:
      return true;

    // Safety and Testing states
    case DependencyAction::PANIC_UNWRAPPED_ANY:
      xbt_die("Critical routing failure: ANY transition reached LUT without being unwrapped.");
    case DependencyAction::PANIC_NOMC:
      xbt_die("We are not supposed to compute the dependency of the NOMC transitions");
    case DependencyAction::EVAL_DYNAMIC_UNKNOWN:
      // Since we swapped to t1 <= t2 and UNKNOWN is the highest enum value,
      // t2 is guaranteed to be the UNKNOWN transition (or both are).
      return t2->depends(t1);

    // Delegate explicitly via virtual calls when the depends function uses private fields
    case DependencyAction::EVAL_BARRIER_DEPENDS:
      return static_cast<const BarrierTransition*>(t1)->depends(t2);
    case DependencyAction::EVAL_OBJECT_DEPENDS:
      return static_cast<const ObjectAccessTransition*>(t1)->depends(t2);

    // Actor actions
    case DependencyAction::EVAL_T1_ACTOR_JOIN:
      return static_cast<const ActorJoinTransition*>(t1)->get_target() == t2->aid_;

    case DependencyAction::EVAL_T2_ACTOR_JOIN:
      return static_cast<const ActorJoinTransition*>(t2)->get_target() == t1->aid_;

    case DependencyAction::EVAL_BOTH_ACTOR_JOIN:
      return static_cast<const ActorJoinTransition*>(t1)->get_target() == t2->aid_ ||
             static_cast<const ActorJoinTransition*>(t2)->get_target() == t1->aid_;

    case DependencyAction::EVAL_T1_ACTOR_CREATE:
      return static_cast<const ActorCreateTransition*>(t1)->get_child() == t2->aid_;

    case DependencyAction::EVAL_T2_ACTOR_CREATE:
      return static_cast<const ActorCreateTransition*>(t2)->get_child() == t1->aid_;

    case DependencyAction::EVAL_BOTH_ACTOR_CREATE:
      return true; // Compete for the same child PIDs

    case DependencyAction::EVAL_CONDVAR_WAKEUP:
      // CondvarAsyncLock and Signals/Broadcast are dependent with each other
      return static_cast<const CondvarTransition*>(t1)->get_condvar() ==
             static_cast<const CondvarTransition*>(t2)->get_condvar();

    case DependencyAction::EVAL_CONDVAR_WAIT_MUTEX:
      // A Wait is dependent with another Wait since it implicitly inserts a mutex_async_lock
      return static_cast<const CondvarTransition*>(t1)->get_mutex() ==
             static_cast<const CondvarTransition*>(t2)->get_mutex();

    case DependencyAction::EVAL_CONDVAR_MUTEX_WAIT:
      // Condvar_wait behaves functionally as a mutex_async_lock
      return static_cast<const MutexTransition*>(t1)->get_mutex() ==
             static_cast<const CondvarTransition*>(t2)->get_mutex();

    case DependencyAction::EVAL_CONDVAR_MUTEX_UNLOCK:
      // Condvar_async_lock behaves functionally as a mutex_unlock
      return static_cast<const MutexTransition*>(t1)->get_mutex() ==
             static_cast<const CondvarTransition*>(t2)->get_mutex();

    case DependencyAction::EVAL_MUTEX_ID:
      return static_cast<const MutexTransition*>(t1)->get_mutex() ==
             static_cast<const MutexTransition*>(t2)->get_mutex();

    case DependencyAction::EVAL_SEM_ID:
      return static_cast<const SemaphoreTransition*>(t1)->get_sem() ==
             static_cast<const SemaphoreTransition*>(t2)->get_sem();

    case DependencyAction::EVAL_COMM_RECV_RECV:
      return static_cast<const CommRecvTransition*>(t1)->get_mailbox() ==
             static_cast<const CommRecvTransition*>(t2)->get_mailbox();

    case DependencyAction::EVAL_COMM_RECV_IPROBE:
      return static_cast<const CommRecvTransition*>(t1)->get_mailbox() ==
             static_cast<const CommIprobeTransition*>(t2)->get_mailbox();

    case DependencyAction::EVAL_COMM_SEND_SEND:
      return static_cast<const CommSendTransition*>(t1)->get_mailbox() ==
             static_cast<const CommSendTransition*>(t2)->get_mailbox();

    case DependencyAction::EVAL_COMM_SEND_IPROBE:
      return static_cast<const CommSendTransition*>(t1)->get_mailbox() ==
             static_cast<const CommIprobeTransition*>(t2)->get_mailbox();

    case DependencyAction::EVAL_COMM_IPROBE_MBOX:
      return static_cast<const CommIprobeTransition*>(t1)->get_mailbox() ==
             static_cast<const CommIprobeTransition*>(t2)->get_mailbox();

    case DependencyAction::EVAL_COMM_TEST_WAIT:
      // Timeouts are not considered by the independence theorem, thus assumed dependent
      return static_cast<const CommWaitTransition*>(t2)->get_timeout();

    case DependencyAction::EVAL_COMM_WAIT_WAIT:
      return static_cast<const CommWaitTransition*>(t1)->get_timeout() ||
             static_cast<const CommWaitTransition*>(t2)->get_timeout();

    case DependencyAction::EVAL_COMM_RECV_TEST: {
      const auto* r = static_cast<const CommRecvTransition*>(t1);
      const auto* t = static_cast<const CommTestTransition*>(t2);
      if (r->get_mailbox() != t->get_mailbox())
        return false;
      if ((r->aid_ != t->get_sender()) && (r->aid_ != t->get_receiver()))
        return false;
      // If the test is checking a paired comm already, we're independent
      // If we happen to make up that pair, then we're dependent...
      return t->get_comm() == r->get_comm();
    }

    case DependencyAction::EVAL_COMM_RECV_WAIT: {
      const auto* r = static_cast<const CommRecvTransition*>(t1);
      const auto* w = static_cast<const CommWaitTransition*>(t2);
      if (w->get_timeout())
        return true;
      if (r->get_mailbox() != w->get_mailbox())
        return false;
      if ((r->aid_ != w->get_sender()) && (r->aid_ != w->get_receiver()))
        return false;
      if ((r->aid_ != w->aid_) && w->get_comm() != r->get_comm())
        return false;
      return true;
    }

    case DependencyAction::EVAL_COMM_SEND_TEST: {
      const auto* s = static_cast<const CommSendTransition*>(t1);
      const auto* t = static_cast<const CommTestTransition*>(t2);
      if (s->get_mailbox() != t->get_mailbox())
        return false;
      if ((s->aid_ != t->get_sender()) && (s->aid_ != t->get_receiver()))
        return false;
      return t->get_comm() == s->get_comm();
    }

    case DependencyAction::EVAL_COMM_SEND_WAIT: {
      const auto* s = static_cast<const CommSendTransition*>(t1);
      const auto* w = static_cast<const CommWaitTransition*>(t2);
      if (w->get_timeout())
        return true;
      if (s->get_mailbox() != w->get_mailbox())
        return false;
      if ((s->aid_ != w->get_sender()) && (s->aid_ != w->get_receiver()))
        return false;
      if ((s->aid_ != w->aid_) && w->get_comm() != s->get_comm())
        return false;
      return true;
    }
  }

  xbt_die("Critical LUT evaluation failure: Control flow reached end of switch.");
#else
  xbt_die("Computing dependencies between transitions is only interesting in MC mode.");
#endif
}

// boost::intrusive_ptr<Transition> support:
void intrusive_ptr_add_ref(Transition* transition)
{
  transition->refcount_.fetch_add(1, std::memory_order_acq_rel);
}

void intrusive_ptr_release(Transition* transition)
{
  if (transition->refcount_.fetch_sub(1, std::memory_order_acq_rel) == 1)
    delete transition;
}

} // namespace simgrid::mc