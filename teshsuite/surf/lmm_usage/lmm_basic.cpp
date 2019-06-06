#include "src/include/catch.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/surf_interface.hpp"
#include "xbt/log.h"

#define DOUBLE_PRECISION 0.00001

// Namespace shortcut
namespace lmm = simgrid::kernel::lmm;

// XBT_LOG_NEW_DEFAULT_CATEGORY(surf_test, "Messages specific for surf example");
// XBT_INFO("V1=%g V2=%g V3=%g",V1->get_value(),V2->get_value(),V3->get_value());

TEST_CASE("Test on small systems", "[lmm-small-systems]")
{
  lmm::System* Sys = lmm::make_new_maxmin_system(false);

  SECTION("Check variables weight (sharing_weight)")
  {

    // Create constraint
    lmm::Constraint* C = Sys->constraint_new(nullptr, 10);

    // Create the 3 variables (with increasing weight from 1 to 3)
    lmm::Variable* V1 =
        Sys->variable_new(nullptr, 1, 0.0, 1); // id=nullptr, weight_value=1, bound=0, number_of_constraints=1
    lmm::Variable* V2 = Sys->variable_new(nullptr, 2, 0.0, 1);
    lmm::Variable* V3 = Sys->variable_new(nullptr, 3, 0.0, 1);

    // Link Variables to Constraint with constant weight 1
    Sys->expand(C, V1, 1);
    Sys->expand(C, V2, 1);
    Sys->expand(C, V3, 1);

    Sys->solve();

    // Compare with theorical values
    REQUIRE(double_equals(V1->get_value(), 5.45455, DOUBLE_PRECISION));
    REQUIRE(double_equals(V2->get_value(), 2.72727, DOUBLE_PRECISION));
    REQUIRE(double_equals(V3->get_value(), 1.81818, DOUBLE_PRECISION));
  }
}
