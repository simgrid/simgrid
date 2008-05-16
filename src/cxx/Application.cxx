#include <Application.hpp>
#include <ApplicationHandler.hpp>

#include <sys/types.h>
#include <sys/stat.h>

#ifndef S_ISREG
	#define S_ISREG(__mode) (((__mode) & S_IFMT) == S_IFREG)
#endif

namespace SimGrid
{
	namespace Msg
	{
	
		Application::Application()
		{
			this->file = NULL;
			this->deployed = false;
		}
				
		Application(const Application& rApplication)
		{
				
			this->file = rApplication.getFile();
			this->deployed = rApplication.isDeployed();
		}
	
		Application::Application(const char* file)
		throw(InvalidParameterException)
		{
			// check parameters
			
			if(!file)
				throw InvalidParameterException("file (must not be NULL");
			
			struct stat statBuf = {0};
				
			if(stat(statBuff, &info) < 0 || !S_ISREG(statBuff.st_mode))
				throw InvalidParameterException("file (file not found)");
				
			this->file = file;
			this->deployed = false;
		}
		
		Application::~Application()
		{
			// NOTHING TODO
		}
			
		Application::deploy(const char* file)
		throw(InvalidParameterException, LogicException, MsgException)
		{
			// check logic
			
			if(this->deployed)
				throw LogicException("application already deployed");
			
			// check the parameters
				
			if(!file)
				throw InvalidParameterException("file (must not be NULL");
			
			struct stat statBuf = {0};
				
			if(stat(statBuff, &info) < 0 || !S_ISREG(statBuff.st_mode))
				throw InvalidParameterException("file (file not found)");
					
			surf_parse_reset_parser();
  			surfxml_add_callback(STag_surfxml_process_cb_list, ApplicationHandler::onBeginProcess);
  			surfxml_add_callback(ETag_surfxml_argument_cb_list, ApplicationHandler::onArg);
  			surfxml_add_callback(STag_surfxml_prop_cb_list, ApplicationHandler::OnProperty);
  			surfxml_add_callback(ETag_surfxml_process_cb_list, ApplicationHandler::OnEndProcess);

  			surf_parse_open(file);
  			
  			if(surf_parse())
  				throw MsgException("surf_parse() failed");
  			
  			surf_parse_close();	
  			
  			this->file = file;
  			this->deployed = true;
		}
		
		Application::deploy(void)
		throw(LogicException, MsgException)
		{
			// check logic
			
			if(this->deployed)
				throw LogicException("application already deployed");
			
			// check the parameters
			if(!this->file)
				throw LogicException("you must specify the xml file which describe the application\nuse Application::setFile()"); 
			
			surf_parse_reset_parser();
  			surfxml_add_callback(STag_surfxml_process_cb_list, ApplicationHandler::onBeginProcess);
  			surfxml_add_callback(ETag_surfxml_argument_cb_list, ApplicationHandler::onArg);
  			surfxml_add_callback(STag_surfxml_prop_cb_list, ApplicationHandler::OnProperty);
  			surfxml_add_callback(ETag_surfxml_process_cb_list, ApplicationHandler::OnEndProcess);

  			surf_parse_open(file);
  			
  			if(surf_parse())
  				throw MsgException("surf_parse() failed");
  			
  			surf_parse_close();
  			
  			this->deployed = true;	
		}
		
		bool Application::isDeployed(void)
		{
			return this->deployed;
		}
		
		void Application::setFile(const char* file)
		throw (InvalidParameterException, LogicException)
		{
			// check logic
			
			if(this->deployed)
				throw LogicException("your are trying to change the file of an already deployed application");
				
			// check parameters
			
			if(!file)
				throw InvalidParameterException("file (must not be NULL");
			
			struct stat statBuf = {0};
				
			if(stat(statBuff, &info) < 0 || !S_ISREG(statBuff.st_mode))
				throw InvalidParameterException("file (file not found)");
				
			this->file = file;
			
		}
		
		const char* Application::getFile(void) const
		{
			return this->file;
		}
		
		const Application& Application::operator = (const Application& rApplication)
		throw(LogicException)
		{
			// check logic
			
			if(this->deployed)
				throw LogicException("application already deployed");
			
			this->file = rApplication.getFile();
			this->deployed = rApplication.isDeployed();
			
			return *this;
		}
	} // namespace Msg
} // namespace SimGrid