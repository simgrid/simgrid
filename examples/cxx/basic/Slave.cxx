#include "Slave.hpp"
#include "FinalizeTask.hpp"
#include <Host.hpp>
#include <HostNotFoundException.hpp>

#include <iostream>
using namespace std;

MSG_IMPLEMENT_DYNAMIC(Slave, Process);

int Slave::main(int argc, char** argv)
{
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Hello I'm " << getName() << " on " << getHost().getName() << "!" << endl;
	
	while(true) 
	{ 
		Task* t = Task::get(0);	
	
		if(t->isInstanceOf("FinalizeTask")) 
		{
			delete t;
			break;
		}
		
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Received \"" << t->getName() << "\" " << endl;
	
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Processing \"" << t->getName() <<  "\" " << endl;

		t->execute();
		
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "\"" << t->getName() << "\" done " << endl;

		delete t;
	}
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Received Finalize. I'm done. See you!" << endl;
	
	delete this;

	return 0;
}