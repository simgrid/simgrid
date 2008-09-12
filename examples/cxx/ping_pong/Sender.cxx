#include "Sender.hpp"
#include "PingPongTask.hpp"
#include <MsgHost.hpp>
#include <HostNotFoundException.hpp>
#include <Msg.hpp>

#include <Msg.hpp>
using namespace std;

const double commSizeLat = 1;
const double commSizeBw = 100000000;

MSG_IMPLEMENT_DYNAMIC(Sender, Process)

int Sender::main(int argc, char** argv)
{
	info("Hello");
	
	int hostCount = argc;
	
	info(TEXT_("Host count : ") + TEXT_(hostCount));
	
	Host* hostTable = new Host[hostCount]; 
	double time;
	double computeDuration = 0;
	PingPongTask* task;
	
	for(int pos = 0; pos < argc ; pos++) 
	{
		try 
		{
			hostTable[pos] = Host::getByName(argv[pos]);
		} 
		catch(HostNotFoundException e) 
		{
			error(TEXT_(e.toString()) + TEXT_(". Stopping Now!"));
			exit(1);
		}
	}
	
	for (int pos = 0; pos < hostCount; pos++) 
	{ 
		time = getClock(); 
		
		info(TEXT_("Sender time : ") + TEXT_(time));
	
		task = new PingPongTask("no name",computeDuration,commSizeLat);
		task->setTime(time);
	
		hostTable[pos].put(0,task);
	} 
	
	info("Goodbye!");

	
	delete[] hostTable;

	delete this;

	return 0;
    
}

