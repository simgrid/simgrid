#include "Sender.hpp"
#include "PingPongTask.hpp"
#include <Host.hpp>
#include <HostNotFoundException.hpp>
#include <Msg.hpp>

#include <iostream>
using namespace std;

const double commSizeLat = 1;
const double commSizeBw = 100000000;

MSG_IMPLEMENT_DYNAMIC(Sender, Process);

int Sender::main(int argc, char** argv)
{
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Hello I'm " << getName() << " on " << getHost().getName() << "!" << endl;
	
	int hostCount = argc;
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " <<  "host count : " << hostCount << endl;
	
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
			cerr << e.toString() <<". Stopping Now!" << endl;
			exit(1);
		}
	}
	
	for (int pos = 0; pos < hostCount; pos++) 
	{ 
		time = getClock(); 
	
		cout <<"[" << getName() << ":" << getHost().getName() << "] " <<  "sender time : " << time << endl;
	
		task = new PingPongTask("no name",computeDuration,commSizeLat);
		task->setTime(time);
	
		hostTable[pos].put(0,task);
	} 
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " <<  "goodbye!" << endl;
	
	delete[] hostTable;

	delete this;

	return 0;
    
}