#ifndef MSG_ENVIRONMENT_HPP
#define MSG_ENVIRONMENT_HPP

#ifndef __cplusplus
	#error Sumulation.hpp requires C++ compilation (use a .cxx suffix)
#endif

namespace SimGrid
{
	namespace Msg
	{
		class Simulation
		{
			public :
			
				Simulation();
				
				Simulation(const Simulation& rSimulation);
				
				virtual ~Simulation();
				
			// Operations.
			
				int execute(int argc, char** argv);
				
				void run(const Environment& rEnvironment, const Application& rApplication)
				throw (MsgException);
				
				
			// Operators.
			
				const Simulation& operator = (const Simulation& rSimulation);
			
			
		};
		
	} // namespace Msg
} // namespace SimGrid


#endif // !MSG_ENVIRONMENT_HPP