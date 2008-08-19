/*
 * Simulation.hpp
 *
 * This file contains the declaration of the wrapper class of the native MSG task type.
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
 
#ifndef MSG_SIMULATION_HPP
#define MSG_SIMULATION_HPP

#ifndef __cplusplus
	#error Sumulation.hpp requires C++ compilation (use a .cxx suffix)
#endif

#include <MsgException.hpp>
#include <FileNotFoundException.hpp>

namespace SimGrid
{
	namespace Msg
	{
		class MsgException;
		class FileNotFoundException;

		// Simulation class declaration.
		class SIMGRIDX_EXPORT Simulation
		{
			public :
			
				Simulation(){};
				
				Simulation(const Simulation& rSimulation);
				
				virtual ~Simulation(){};
				
			// Operations.
			
				int execute(int argc, char** argv);
				
			private:
				
				void run(void)
				throw (MsgException);
				
				
			// Operators.
				const Simulation& operator = (const Simulation& rSimulation);
		};
		
	} // namespace Msg
} // namespace SimGrid

#endif // !MSG_SIMULATION_HPP

