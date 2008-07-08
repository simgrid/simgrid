#include "Master.hpp"
#include "CommTimeTask.hpp"
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
	
	sscanf(argv[0],"%d", &numberOfTasks);
	
	sscanf(argv[1],"%lg", &taskComputeSize);
	
	sscanf(argv[2],"%lg", &taskCommunicateSize);
	
	
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
		sprintf(buff,"Task_%d",i);
		CommTimeTask* task = new CommTimeTask(buff, taskComputeSize, taskCommunicateSize);
		task->setTime(getClock());
		slaves[i % slaveCount].put(0, task);
		memset(buff, 0 , BUFFMAX + 1);
	}
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "All tasks have been dispatched. Let's tell everybody the computation is over." << endl;
	
	for (int i = 0; i < slaveCount; i++) 
	{ 
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Finalize host " << slaves[i].getName() <<  " [" <<  i  << "]" << endl;
		slaves[i].put(0, new FinalizeTask());
	}
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "All finalize messages have been dispatched. Goodbye now!" << endl;
	

	delete[] slaves;
	delete this;
	return 0;
}