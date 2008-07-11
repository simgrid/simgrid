#include "Receiver.hpp"
#include "PingPongTask.hpp"
#include <Host.hpp>
#include <HostNotFoundException.hpp>
#include <Msg.hpp>

#include <Msg.hpp>

MSG_IMPLEMENT_DYNAMIC(Receiver, Process);

const double commSizeLat = 1;
const double commSizeBw = 100000000;

int Receiver::main(int argc, char** argv)
{
	info("Hello");
	double communicationTime=0;
	double time = getClock();
	StringHelper bw;
	
	info("Try to get a task");
	
	PingPongTask* task = reinterpret_cast<PingPongTask*>(Task::get(0));
	
	double timeGot = getClock();
	double timeSent = task->getTime();
	
	delete task;
	
	info(TEXT_("Got at time ") + TEXT_(timeGot));
	info(TEXT_("Was sent at time ") + TEXT_(timeSent));
	
	time = timeSent;
	
	communicationTime = timeGot - time;
	
	info(TEXT_( "Communication time : ") + TEXT_(communicationTime));
	
	info(TEXT_(" --- BW ") + bw.append(commSizeBw/communicationTime,"%07lf") + TEXT_(" ----"));
	
	info("Goodbye!");
	
	delete this;

	return 0;
}