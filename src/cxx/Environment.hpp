#ifndef MSG_ENVIRONMENT_HPP
#define MSG_ENVIRONMENT_HPP

#ifndef __cplusplus
	#error Environment.hpp requires C++ compilation (use a .cxx suffix)
#endif

#include <InvalidParameterException.hpp>
#include <LogicException.hpp>
#include <MsgException.hpp>


namespace SimGrid
{
	namespace Msg
	{
		class Environment
		{
			public:
				
				Environment();
				
				Environment(const Environment& rEnvironment);
				
				Environment(const char* file)
				throw(InvalidArgumentException);
				
				virtual ~Environment();
				
				// Operations.
				
				void load(void)
				throw(LogicException);
				
				void load(const char* file)
				throw(InvalidArgumentException, LogicException);
				
				bool isLoaded(void) const;
				
				// Getters/setters
				void setFile(const char* file)
				throw(InvalidArgumentException, LogicException);
				
				const char* getFile(void) const;
				
				// Operators.
				
				const Environment& operator = (const Environment& rEnvironment)
				throw(LogicException);
				
			private:
				
				// Attributes.
				
				// the xml file which describe the environment of the simulation.
				const char* file;
				
				// flag : is true the environment of the simulation is loaded.
				bool loaded.
		};
		
	} // namespace Msg
} // namespace SimGrid


#endif // !MSG_ENVIRONMENT_HPP