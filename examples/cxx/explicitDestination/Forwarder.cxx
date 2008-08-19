#include "Forwarder.hpp"
#include "BasicTask.hpp"
#include "FinalizeTask.hpp"

#include <Host.hpp>
#include <HostNotFoundException.hpp>

#include <Msg.hpp>


MSG_IMPLEMENT_DYNAMIC(Forwarder, Process)


int Forwarder::main(int argc, char** argv)
{
	info("Hello");
	
	int aliasCount = argc;
	
	int taskCount = 0;
	
	Task* taskReceived;
	Task* finalizeTask;
	BasicTask* basicTask;
	
	while(true) 
	{
		taskReceived = Task::receive(Host::currentHost().getName());
	
		if(taskReceived->isInstanceOf("FinalizeTask")) 
		{
			info("All tasks have been dispatched. Let's tell everybody the computation is over.");
	
			for (int i = 0; i < aliasCount; i++) 
			{
				finalizeTask = new FinalizeTask();
				finalizeTask->send(argv[i]);
			}
			
			delete taskReceived;
	
			break;
		}
	
		basicTask = reinterpret_cast<BasicTask*>(taskReceived);
	
		info(TEXT_("Received \"") + TEXT_(basicTask->getName()) + TEXT_("\" "));
	
		info(TEXT_("Sending \"") + TEXT_(basicTask->getName()) + TEXT_("\" to \"") + TEXT_(argv[taskCount % aliasCount]) + TEXT_("\""));
	
		basicTask->send(argv[taskCount % aliasCount]);
	
		taskCount++;
	}
	
	
	info("I'm done. See you!");
	
	delete this;
	return 0;
}
