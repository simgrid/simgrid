#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(main, "Messages specific for this s4u example");

int main(int argc, char* argv[]) {
    simgrid::s4u::Engine e(&argc, argv);
    e.load_platform(argv[1]);

    std::vector<simgrid::s4u::ActivityPtr> dag = simgrid::s4u::create_DAG_from_dot(argv[2]);

    simgrid::s4u::Host* tremblay = e.host_by_name("Tremblay");
    simgrid::s4u::Host* jupiter  = e.host_by_name("Jupiter");
    simgrid::s4u::Host* fafard  = e.host_by_name("Fafard");

    dynamic_cast<simgrid::s4u::Exec*>(dag[0].get())->set_host(fafard);
    dynamic_cast<simgrid::s4u::Exec*>(dag[1].get())->set_host(tremblay);
    dynamic_cast<simgrid::s4u::Exec*>(dag[2].get())->set_host(jupiter);
    dynamic_cast<simgrid::s4u::Exec*>(dag[3].get())->set_host(jupiter);
    dynamic_cast<simgrid::s4u::Exec*>(dag[8].get())->set_host(jupiter);
    
    for (const auto& a : dag) {
        if (auto* comm = dynamic_cast<simgrid::s4u::Comm*>(a.get())) {
            auto pred = dynamic_cast<simgrid::s4u::Exec*>((*comm->get_dependencies().begin()).get());
            auto succ = dynamic_cast<simgrid::s4u::Exec*>(comm->get_successors().front().get());
            comm->set_source(pred->get_host())->set_destination(succ->get_host());
        }
    }

    simgrid::s4u::Activity::on_completion_cb([](simgrid::s4u::Activity const& activity) {
    XBT_INFO("Activity '%s' is complete (start time: %f, finish time: %f)", activity.get_cname(), activity.get_start_time(),
             activity.get_finish_time());
    });

    e.run();
	return 0;
}

