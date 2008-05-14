#include <ApplicationHandler.hpp>

namespace msg
{
	
ApplicationHandler::ProcessFactory::processFactory = NULL;
	
void ApplicationHandler::onStartDocument(void)
{
	processFactory = new ProcessFactory();	
}

void ApplicationHandler::onEndDocument(void)
{
	if(processFactory)
		delete processFactroy;
}
	
void ApplicationHandler::onBeginProcess(void)
{
	processFactory->setProcessIdentity(A_surfxml_process_host, A_surfxml_process_function);
}		

void ApplicationHandler::onProcessArg(void)
{
	processFactory->registerProcessArg(A_surfxml_argument_value);
}

void ApplicationHandler::OnProperty(void)
{
	 processFactory->setProperty(A_surfxml_prop_id, A_surfxml_prop_value);
}

void ApplicationHandler::onEndProcess(void)
{
	processFactory->createProcess();
}

void ApplicationHandler::ProcessFactory::createProcess() 
	{
    	Process* process = (Process*)Class::fromName(this->function);
   		
   		Host host = Host::getByName(this->hostName);

    	process->create();
    	
    	Strings* args = processFactory->args;
    	
    	int size = args->size();

    	for (int index = 0; index < size; index++)
      		process->args->push_back(args->at(index));
      
    	process->properties = this->properties;
    	this->properties = new Properties();
	}
	
void ApplicationHandler::ProcessFactory::setProcessIdentity(const string& hostName, const string& function) 
{
	this->hostName = hostName;
	this->function = function;

	if (!this->args->empty())
		this->args->clear();

	if(!this->properties->empty())
		this->properties->clear();
}

}