#include "smpi/smpi_utils.hpp"
#include "src/smpi/SmpiHost.hpp"
#include <simgrid/s4u/VirtualMachine.hpp>
#include <string>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_host, smpi, "Logging specific to SMPI (host)");

namespace simgrid {
namespace smpi {

simgrid::xbt::Extension<simgrid::s4u::Host, SmpiHost> SmpiHost::EXTENSION_ID;

double SmpiHost::orecv(size_t size)
{
  if (orecv_parsed_values.empty()) {
    if (orecv_.empty())  { /* This is currently always true since orecv_ is not really used yet. */
      /* Get global value */
      orecv_parsed_values = parse_factor(xbt_cfg_get_string("smpi/or"));
    }
    else /* Can currently not be reached, see above */
      orecv_parsed_values = parse_factor(orecv_.c_str());
  }
  
  double current=orecv_parsed_values.empty()?0.0:orecv_parsed_values.front().values[0]+orecv_parsed_values.front().values[1]*size;
  
  // Iterate over all the sections that were specified and find the right value. (fact.factor represents the interval
  // sizes; we want to find the section that has fact.factor <= size and no other such fact.factor <= size)
  // Note: parse_factor() (used before) already sorts the vector we iterate over!
  for (auto fact : orecv_parsed_values) {
    if (size <= fact.factor) { // Values already too large, use the previously computed value of current!
      XBT_DEBUG("or : %zu <= %zu return %.10f", size, fact.factor, current);
      return current;
    } else {
      // If the next section is too large, the current section must be used.
      // Hence, save the cost, as we might have to use it.
      current=fact.values[0]+fact.values[1]*size;
    }
  }
  XBT_DEBUG("smpi_or: %zu is larger than largest boundary, return %.10f", size, current);

  return current;
}

SmpiHost::SmpiHost(simgrid::s4u::Host *ptr) : host(ptr)
{
  if (!SmpiHost::EXTENSION_ID.valid())
    SmpiHost::EXTENSION_ID = simgrid::s4u::Host::extension_create<SmpiHost>();

 // if (host->properties() != nullptr) {
 //   char* off_power_str = (char*)xbt_dict_get_or_null(host->properties(), "watt_off");
 //   if (off_power_str != nullptr) {
 //     char *msg = bprintf("Invalid value for property watt_off of host %s: %%s",host->name().c_str());
 //     watts_off = xbt_str_parse_double(off_power_str, msg);
 //     xbt_free(msg);
 //   }
 //   else
 //     watts_off = 0;
 // }
}

SmpiHost::~SmpiHost()=default;

static void onCreation(simgrid::s4u::Host& host) {
}

static void onHostDestruction(simgrid::s4u::Host& host) {
  // Ignore virtual machines
  if (dynamic_cast<simgrid::s4u::VirtualMachine*>(&host))
    return;
}

void sg_smpi_host_init()
{
  simgrid::s4u::Host::onCreation.connect(&onCreation);
  simgrid::s4u::Host::onDestruction.connect(&onHostDestruction);
}

}
}
