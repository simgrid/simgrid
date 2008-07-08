#include "Master.hpp"
#include "BasicTask.hpp"
#include "FinalizeTask.hpp"

#include <Host.hpp>
#include <HostNotFoundException.hpp>

#include <iostream>
using namespace std;


#ifndef BUFFMAX
#define BUFFMAX 260
#endif

MSG_IMPLEMENT_DYNAMIC(Master, Process);

int Master::main(int argc, char** argv)
{
	char buff[BUFFMAX + 1] = {0};
	int taskCount;		
	double taskComputeSize;		
	double taskCommunicateSize;
	
	cout << "Hello I'm " << getName() << " on " << getHost().getName() << "!" << endl;
		
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "argc=" << argc << endl;
		
	for (int i = 0; i< argc; i++)	    
		cout << "argv:" << argv[i] << endl;
	
	sscanf(argv[0],"%d", &taskCount);
	sscanf(argv[1],"%lg", &taskComputeSize);
	sscanf(argv[2],"%lg", &taskCommunicateSize);
	
	BasicTaskPtr* basicTasks = new BasicTaskPtr[taskCount];
		
	for (int i = 0; i < taskCount; i++) 
	{
		sprintf(buff,"Task_%d",i);
		basicTasks[i] = new BasicTask(buff, taskComputeSize, taskCommunicateSize);
		memset(buff, 0 , BUFFMAX + 1); 
	}		
	
	int aliasCount = argc - 3;
		
	char** aliases = (char**) calloc(aliasCount, sizeof(char*));
		
	for(int i = 3; i < argc ; i++) 
		aliases[i - 3] = _strdup(argv[i]);
		
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Got "<<  aliasCount << " alias(es) :" << endl;
		
	for (int i = 0; i < aliasCount; i++)
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "\t" << aliases[i] << endl;
		
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Got "<< taskCount << " task to process." << endl;
		
	for (int i = 0; i < taskCount; i++) 
	{	
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Sending \"" << basicTasks[i]->getName() << "\" to \"" << aliases[i % aliasCount] << "\"" << endl;
		
		/*if((Host::currentHost().getName()).equals((aliases[i % aliasCount].split(":"))[0]))
			cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Hey ! It's me ! ";
		*/
			
		basicTasks[i]->send(aliases[i % aliasCount]);
	}
		
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Send completed" << endl;
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "All tasks have been dispatched. Let's tell everybody the computation is over." << endl;
	
	FinalizeTask* finalizeTask;
	
	for (int i = 0; i < aliasCount; i++) 
	{
		finalizeTask = new FinalizeTask();
		finalizeTask->send(aliases[i]);
		
	}
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Goodbye now!" << endl;

	delete[] basicTasks;
	delete[] aliases;

	delete this;

	return 0;
}