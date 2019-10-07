#include "simgrid/s4u.hpp"
#include "smpi/smpi.h"
#include "xbt/asserts.h"
#include "xbt/replay.hpp"
#include "xbt/str.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_replay);

int main(int argc, char* argv[])
{
  if (simgrid::s4u::Actor::self() == nullptr) {
    XBT_ERROR("smpireplaymain should not be called directly. Please use smpirun -replay instead.");
    return 1;
  }

  auto properties = simgrid::s4u::Actor::self()->get_properties();
  if (properties->find("smpi_replay") == properties->end()) {
    XBT_ERROR("invalid smpireplaymain execution. Please use smpirun -replay instead.");
    return 1;
  }

  const char* instance_id    = properties->at("instance_id").c_str();
  const int rank             = xbt_str_parse_int(properties->at("rank").c_str(), "Cannot parse rank");
  const char* trace_filename = argv[1];
  double start_delay_flops   = 0;

  if (argc > 2) {
    start_delay_flops = xbt_str_parse_double(argv[2], "Cannot parse start_delay_flops");
  }

  /* Setup things and register default actions */
  smpi_replay_init(instance_id, rank, start_delay_flops);

  /* A small check, just to make sure SMPI ranks are consistent with input arguments */
  int new_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &new_rank);
  xbt_assert(new_rank == rank, "Rank inconsistency. Got %d, expected %d", new_rank, rank);

  /* The regular run of the replayer */
  smpi_replay_main(rank, trace_filename);
  return 0;
}
