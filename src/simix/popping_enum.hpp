/**********************************************************************/
/* File generated by src/simix/simcalls.py from src/simix/simcalls.in */
/*                                                                    */
/*                    DO NOT EVER CHANGE THIS FILE                    */
/*                                                                    */
/* change simcalls specification in src/simix/simcalls.in             */
/* Copyright (c) 2014-2021. The SimGrid Team. All rights reserved.    */
/**********************************************************************/

/*
 * Note that the name comes from http://en.wikipedia.org/wiki/Popping
 * Indeed, the control flow is doing a strange dance in there.
 *
 * That's not about http://en.wikipedia.org/wiki/Poop, despite the odor :)
 */

namespace simgrid {
namespace simix {
/**
 * @brief All possible simcalls.
 */
enum class Simcall {
  NONE,
  EXECUTION_WAITANY_FOR,
  COMM_RECV,
  COMM_IRECV,
  COMM_SEND,
  COMM_ISEND,
  COMM_TEST,
  COMM_TESTANY,
  COMM_WAITANY,
  COMM_WAIT,
  RUN_KERNEL,
  RUN_BLOCKING,
};

constexpr int NUM_SIMCALLS = 12;
} // namespace simix
} // namespace simgrid
