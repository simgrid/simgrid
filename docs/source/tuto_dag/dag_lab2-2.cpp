#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(main, "Messages specific for this s4u example");

int main(int argc, char* argv[]) {
    simgrid::s4u::Engine e(&argc, argv);
    e.load_platform(argv[1]);

    std::vector<simgrid::s4u::ActivityPtr> dag = simgrid::s4u::create_DAG_from_json(argv[2]);

    simgrid::s4u::Activity::on_completion_cb([](simgrid::s4u::Activity const& activity) {
    XBT_INFO("Activity '%s' is complete (start time: %f, finish time: %f)", activity.get_cname(), activity.get_start_time(),
             activity.get_finish_time());
    });

    e.run();
	return 0;
}

