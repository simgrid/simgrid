#include "Slave.hpp"
#include "FinalizeTask.hpp"
#include "BasicTask.hpp"

#include <Host.hpp>
#include <HostNotFoundException.hpp>

#include <iostream>
using namespace std;

MSG_IMPLEMENT_DYNAMIC(Slave, Process);

int Slave::main(int argc, char** argv)
{
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Hello I'm " << getName() << " on " << getHost().getName() << "!" << endl;
	
	Task* receivedTask;
	BasicTask* basicTask;
	
	while(true) 
	{ 
		receivedTask = Task::receive(Host::currentHost().getName());	
	
		if(receivedTask->isInstanceOf("FinalizeTask")) 
		{
			delete receivedTask;
			break;
		}
	
		basicTask = reinterpret_cast<BasicTask*>(receivedTask);
	
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Received \"" << basicTask->getName() << "\" " << endl;
	
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Processing \"" << basicTask->getName() <<  "\" " << endl;	 
		basicTask->execute();
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "\"" << basicTask->getName() << "\" done " << endl;
	
		delete basicTask;
	}
		
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Received Finalize. I'm done. See you!" << endl;

	return 0;
}