#include "Receiver.hpp"
#include "PingPongTask.hpp"
#include <Host.hpp>
#include <HostNotFoundException.hpp>
#include <Msg.hpp>

#include <iostream>
using namespace std;

MSG_IMPLEMENT_DYNAMIC(Receiver, Process);

const double commSizeLat = 1;
const double commSizeBw = 100000000;

int Receiver::main(int argc, char** argv)
{
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Hello I'm " << getName() << " on " << getHost().getName() << "!" << endl;
	double communicationTime=0;
	double time = getClock();
	
	cout <<"[" << getName() << ":" << getHost().getName() << "try to get a task" << endl;
	
	PingPongTask* task = reinterpret_cast<PingPongTask*>(Task::get(0));
	
	double timeGot = getClock();
	double timeSent = task->getTime();
	
	delete task;
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Got at time " << timeGot << endl;
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Was sent at time " << timeSent << endl;
	
	time = timeSent;
	
	communicationTime = timeGot - time;
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Communication time : " << communicationTime << endl;
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << " --- bw " << commSizeBw/communicationTime << " ----" << endl;
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "goodbye!" << endl;
	
	delete this;

	return 0;
}