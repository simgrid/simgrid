#include <Simulation.hpp>

#include <Simulator.hpp>

namespace SimGrid
{
	namespace Msg
	{
		int Simulation::execute(int argc, char** argv)
		{
			if(argc < 3) 
			{
		      info("Usage: Msg platform_file deployment_file");
		      return 1;
		    }
		    
			 // initialize the MSG simulator. Must be done before anything else (even logging).
    		Simulator::init(argc, argv);
			
			// the environment to load
			Environment environment(argv[1]);
			// the application to deploy
			Application application(argv[2]);
			// the simulation
			Simulation simulation;
			
			// load the environment
			environment.load();
			
			// deploy the application
			application.deploy();
			
			// run the simulation
			simulation.run(environment, application);
			
			// finalize the simulator
			Simulator::finalize();
		}
	} // namespace Msg
} // namespace SimGrid
