#include "Forwarder.hpp"
#include "BasicTask.hpp"
#include "FinalizeTask.hpp"

#include <Host.hpp>
#include <HostNotFoundException.hpp>

#include <iostream>
using namespace std;

MSG_IMPLEMENT_DYNAMIC(Forwarder, Process);


int Forwarder::main(int argc, char** argv)
{
	cout << "Hello I'm " << getName() << " on " << getHost().getName() << "!" << endl;
	
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
			cout <<"[" << getName() << ":" << getHost().getName() << "] " << "All tasks have been dispatched. Let's tell everybody the computation is over." << endl;
	
			for (int i = 0; i < aliasCount; i++) 
			{
				finalizeTask = new FinalizeTask();
				finalizeTask->send(argv[i]);
			}
			
			delete taskReceived;
	
			break;
		}
	
		basicTask = reinterpret_cast<BasicTask*>(taskReceived);
	
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Received \"" << basicTask->getName() << "\" " << endl;
	
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Sending \"" << basicTask->getName() << "\" to \"" << argv[taskCount % aliasCount] << "\"" << endl;
	
		basicTask->send(argv[taskCount % aliasCount]);
	
		taskCount++;
	}
	
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "I'm done. See you!" << endl;
	
	return 0;
}