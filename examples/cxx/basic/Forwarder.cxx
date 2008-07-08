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
	
	int slavesCount = argc;
	
	Host* slaves = new Host[slavesCount];
	
	for (int i = 0; i < argc; i++) 
	{
		try 
		{	      
			slaves[i] = Host::getByName(argv[i]);
		} 
		catch (HostNotFoundException e) 
		{
			cerr << e.toString() << endl;
			cerr << "Buggy deployment file" << endl;
			exit(1);
		}
	}
	
	int taskCount = 0;

	while(true) 
	{
		Task* t = Task::get(0);	
	
		if(t->isInstanceOf("FinalizeTask")) 
		{
			cout << getName() << ":" << getHost().getName() << "All tasks have been dispatched. Let's tell everybody the computation is over." << endl;
	
			for (int cpt = 0; cpt< slavesCount; cpt++) 
			{
				slaves[cpt].put(0, new FinalizeTask());
			}

			delete t;

			break;
		}

		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Received \"" << t->getName() << "\" " << endl;
	
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Sending \"" << t->getName() << "\" to \"" << slaves[taskCount % slavesCount].getName() <<"\"" << endl;
		
		slaves[taskCount % slavesCount].put(0, t);
	
		taskCount++;
	}
	
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "I'm done. See you!" << endl;
	
	delete[] slaves;

	delete this;
      
	return 0;
}