#include <Simulation.hpp>

using namespace SimGrid::Msg;

int
main(int argc, char* argv[])
{
	Simulation s;
	
	return s.execute(argc, argv);
}
