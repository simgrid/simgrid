#include "Slave.hpp"
#include "FinalizeTask.hpp"
#include "CommTimeTask.hpp"
#include <Host.hpp>
#include <HostNotFoundException.hpp>
#include <Msg.hpp>

#include <iostream>
using namespace std;

MSG_IMPLEMENT_DYNAMIC(Slave, Process);

int Slave::main(int argc, char** argv)
{
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Hello I'm " << getName() << " on " << getHost().getName() << "!" << endl;
	
	while(true) 
	{
		double time1 = getClock();       
		Task* t = Task::get(0);	
		double time2 = getClock();
	
		if(t->isInstanceOf("FinalizeTask")) 
		{
			delete t;
			break;
		}
		
		CommTimeTask* task = reinterpret_cast<CommTimeTask*>(t);
	
		if(time1 < task->getTime())
			time1 = task->getTime();
			
		// cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Processing \"" << task->getName() << "\" " << getHost().getName() <<  " (Communication time : " <<  (time2 - time1) << ")" << endl;
	 
		task->execute();
		
		delete task;
	}
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Received Finalize. I'm done. See you!" << endl;

	return 0;
	
}