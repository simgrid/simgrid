#include "Slave.hpp"
#include "FinalizeTask.hpp"
#include "CommTimeTask.hpp"
#include <Host.hpp>
#include <HostNotFoundException.hpp>
#include <Msg.hpp>

#include <Msg.hpp>

MSG_IMPLEMENT_DYNAMIC(Slave, Process)

int Slave::main(int argc, char** argv)
{
	info("Hello");
	
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
			
		info(TEXT_("Processing \"") + TEXT_(task->getName()) + TEXT_("\" ") + TEXT_(getHost().getName()) + TEXT_(" (Communication time : ") + TEXT_((time2 - time1)) + TEXT_(")"));
	 
		task->execute();
		
		delete task;
	}
	
	info(TEXT_("Received Finalize. I'm done. See you!"));
	
	delete this;
	return 0;
	
}
