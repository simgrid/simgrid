#include <simgrid/s4u.hpp>
#include <vector>

using namespace std;
using namespace simgrid;

static void runner()
{
  auto e                    = s4u::Engine::get_instance();
  simgrid::s4u::Host* host0 = e->host_by_name("c1_0");
  simgrid::s4u::Host* host1 = e->host_by_name("c2_0");

  std::vector<double> comp = {1e6, 1e6};
  std::vector<double> comm = {1, 2, 3, 4};

  vector<simgrid::s4u::Host*> h1 = {host0, host1};
  simgrid::s4u::this_actor::parallel_execute(h1, comp, comm);
}

int main(int argc, char* argv[])
{
  s4u::Engine e(&argc, argv);
  e.set_config("host/model:ptask_L07");

  xbt_assert(argc == 2,
             "\nUsage: %s platform_ok.xml\n"
             "\tor: %s platform_bad.xml\n",
             argv[0], argv[0]);

  const char* platform_file = argv[1];
  e.load_platform(platform_file);

  simgrid::s4u::Actor::create("actor", e.host_by_name("c1_0"), runner);

  e.run();
  return 0;
}
