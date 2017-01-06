#ifndef SMPI_HOST_HPP_
#define SMPI_HOST_HPP_

#include "src/include/smpi/smpi_utils.hpp"

#include <simgrid/s4u/host.hpp>
#include <string>
#include <vector>
#include <xbt/Extendable.hpp>



namespace simgrid {
namespace smpi {

void sg_smpi_host_init();
static void onHostDestruction(simgrid::s4u::Host& host);
static void onCreation(simgrid::s4u::Host& host);

class SmpiHost {

  private:
  std::vector<s_smpi_factor_t> orecv_parsed_values;
  simgrid::s4u::Host *host = nullptr;

  public:
  static simgrid::xbt::Extension<simgrid::s4u::Host, SmpiHost> EXTENSION_ID;

  explicit SmpiHost(simgrid::s4u::Host *ptr);
  ~SmpiHost();

  double wtime;
  double osend;
  double oisend;
  std::string orecv_;
  double orecv(size_t size);

};

}
}
#endif
