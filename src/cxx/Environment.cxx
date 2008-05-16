#include <Environment.hpp>

#include <sys/types.h>
#include <sys/stat.h>

#ifndef S_ISREG
	#define S_ISREG(__mode) (((__mode) & S_IFMT) == S_IFREG)
#endif

namespace SimGrid
{
	namespace Msg
	{
		Environment::Environment()
		{
			this->file = NULL;
			this->loaded = false;
		}
				
		Environment::Environment(const Environment& rEnvironment);
		{
			this->file = rEnvironment.getFile();
			this->loaded = rEnvironment.isLoaded();
		}
		
		Environment::Environment(const char* file)
		throw(InvalidArgumentException);
		{
			// check parameters
			
			if(!file)
				throw InvalidParameterException("file (must not be NULL");
			
			struct stat statBuf = {0};
				
			if(stat(statBuff, &info) < 0 || !S_ISREG(statBuff.st_mode))
				throw InvalidParameterException("file (file not found)");
				
			this->file = file;
			this->loaded = false;
		}
		
		Environment::~Environment()
		{
			// NOTHING TODO
		}
		
		// Operations.
		
		void Environment::load(void)
		throw(LogicException)
		{
			// check logic
			
			if(this->loaded)
				throw LogicException("environement already loaded");
			
			// check the parameters
			if(!this->file)
				throw LogicException("you must specify the xml file which describe the environment to load\nuse Environment::setFile()"); 
			
			MSG_create_environment(file);
  			
  			this->loaded = true;		
		}
		
		void Environment::load(const char* file)
		throw(InvalidArgumentException, LogicException)
		{
			// check logic
			
			if(this->loaded)
				throw LogicException("environment already loaded");
			
			// check the parameters
				
			if(!file)
				throw InvalidParameterException("file (must not be NULL");
			
			struct stat statBuf = {0};
				
			if(stat(statBuff, &info) < 0 || !S_ISREG(statBuff.st_mode))
				throw InvalidParameterException("file (file not found)");
					
			MSG_create_environment(file);
  			
  			this->file = file;
  			this->loaded = true;	
		}
		
		bool Environment::isLoaded(void) const
		{
			return this->loaded;
		}
		
		// Getters/setters
		void Environment::setFile(const char* file)
		throw(InvalidArgumentException, LogicException)
		{
			// check logic
			
			if(this->loaded)
				throw LogicException("your are trying to change the file of an already loaded environment");
				
			// check parameters
			
			if(!file)
				throw InvalidParameterException("file (must not be NULL");
			
			struct stat statBuf = {0};
				
			if(stat(statBuff, &info) < 0 || !S_ISREG(statBuff.st_mode))
				throw InvalidParameterException("file (file not found)");
				
			this->file = file;
		}
		
		const char* Environment::getFile(void) const
		{
			return this->file;
		}
	
		
		const Environment& Environment::operator = (const Environment& rEnvironment)
		throw(LogicException)
		{
			// check logic
			
			if(this->loaded)
				throw LogicException("environment already loaded");
			
			this->file = rEnvironment.getFile();
			this->loaded = rEnvironment.isLoaded();
			
			return *this;
		}
				
	} // namespace Msg
} // namespace SimGrid