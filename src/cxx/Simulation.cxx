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
    		init(argc, argv);
			
			// the environment to load
			Environment environment;
			
			// the application to deploy
			Application application;
			
			// the simulation
			Simulation simulation;
			
			// try to load the environment described by the xml file (argv[1])
			try
			{
				environment.load(argv[1]);
			}
			catch(InvalidArgumentException e)
			{
				info(e.toString());
				finalize();
				return 1;
			}
			catch(LogicException e)
			{
				info(e.toString());
				finalize();
				return 1;
			}
				
			
			// try to deploy the application described by the xml file deployment (argv[2])
			try
			{
				application.deploy(argv[2]);
			catch(InvalidArgumentException e)
			{
				info(e.toString());
				finalize();
				return 1;
			}
			catch(LogicException e)
			{
				info(e.toString());
				finalize();
				return 1;
			}
			
			//try to run the simulation the given application on the given environment
			try
			{
				simulation.run(environment, application);
			}
			catch(MsgException e)
			{
				info(e.toString());
				finalize();
				return 1;
			}
			
			// finalize the MSG simulator
			
			try
			{
				finalize();
			}
			catch(MsgExceptio e)
			{
				info(e.toString())
				return 1;
			}
			
			return 0;
		}
		
		void run(const Environment& rEnvironment, const Application& rApplication)
		throw (MsgException)
		{
			if(MSG_OK != MSG_main())
	  			throw MsgException("MSG_main() failed");
		}
	} // namespace Msg
} // namespace SimGrid
