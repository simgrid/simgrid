#include "Master.hpp"
#include "BasicTask.hpp"
#include "FinalizeTask.hpp"
#include <Host.hpp>
#include <HostNotFoundException.hpp>

#include <Msg.hpp>

#include <iostream>
using namespace std;


#ifndef BUFFMAX
#define BUFFMAX 260
#endif

MSG_IMPLEMENT_DYNAMIC(Master, Process);

int Master::main(int argc, char** argv)
{
	int channel = 0;
	char buff[BUFFMAX + 1] = {0};
	int numberOfTasks;		
	double taskComputeSize;		
	double taskCommunicateSize;

	if (argc < 3) 
	{
		cerr <<"Master needs 3 arguments" << endl;
		exit(1);
	}
	
	cout << "Hello I'm " << getName() << " on " << getHost().getName() << "!" << endl;
	
	int slaveCount = 0;
	Host* slaves = NULL;
	
	cout << "argc=" <<  argc << endl;
	
	for (int i = 0; i< argc; i++)	    
		cout << "argv:" << argv[i] << endl;
	
	sscanf(argv[0],"%d", &numberOfTasks);
	
	sscanf(argv[1],"%lg", &taskComputeSize);
	
	sscanf(argv[2],"%lg", &taskCommunicateSize);
	
	BasicTaskPtr* todo = new BasicTaskPtr[numberOfTasks];
	
	for (int i = 0; i < numberOfTasks; i++) 
	{
		sprintf(buff,"Task_%d",i);
		todo[i] = new BasicTask(buff, taskComputeSize, taskCommunicateSize);
		memset(buff, 0 , BUFFMAX + 1); 
	}
	
	slaveCount = argc - 3;
	slaves = new Host[slaveCount];
	
	for(int i = 3; i < argc ; i++)  
	{
		try 
		{
			slaves[i-3] = Host::getByName(argv[i]);
		}
		
		catch(HostNotFoundException e) 
		{
			cerr << e.toString() <<". Stopping Now!" << endl;
			exit(1);
		}
	}
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Got slave(s) :"  << slaveCount << endl;
	
	for (int i = 0; i < slaveCount; i++)
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "\t" << slaves[i].getName() << endl;
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Got " << numberOfTasks << " task to process." << endl;
	
	for (int i = 0; i < numberOfTasks; i++) 
	{
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Sending \"" << todo[i]->getName() << "\" to \"" << slaves[i % slaveCount].getName() << "\"" << endl;
	
		if(!strcmp(Host::currentHost().getName(), slaves[i % slaveCount].getName())) 
			cout <<"[" <<  getName() << ":" << getHost().getName() << "] " << "Hey ! It's me ! ";

		slaves[i % slaveCount].put(channel, todo[i]);
	}
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Send completed" << endl;
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "All tasks have been dispatched. Let's tell everybody the computation is over." << endl;
	
	for (int i = 0; i < slaveCount; i++) 
			slaves[i].put(channel, new FinalizeTask());

	delete[] todo;
	delete[] slaves;
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Goodbye now!" << endl;

	delete this;

	return 0;
}