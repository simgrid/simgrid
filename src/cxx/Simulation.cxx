/*
 * Simulation.cxx
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */
 
 /* Simulation member functions implementation.
  */  
  
#include <Simulation.hpp>

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
			Environment env;
			
			// the application to deploy
			Application app;

			
			// try to load the environment described by the xml file (argv[1])
			try
			{
				env.load(argv[1]);
			}
			catch(FileNotFoundException e)
			{
				info(e.toString());
				finalize();
				return 1;
			}
				
			// try to deploy the application described by the xml file deployment (argv[2])
			try
			{
				app.deploy(argv[2]);
			catch(FileNotFoundException e)
			{
				info(e.toString());
				finalize();
				return 1;
			}
			
			//try to run the simulation the given application on the given environment
			try
			{
				this->run();
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
			catch(MsgException e)
			{
				info(e.toString())
				return 1;
			}
			
			return 0;
		}
		
		void Simulation::run(void)
		throw (MsgException)
		{
			if(MSG_OK != MSG_main())
	  			throw MsgException("MSG_main() failed");
		}
	} // namespace Msg
} // namespace SimGrid
