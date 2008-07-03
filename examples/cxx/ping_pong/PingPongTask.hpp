#ifndef PINGPONG_TASK_HPP
#define PINGPONG_TASK_HPP

#include <Task.hpp>
using namespace SimGrid::Msg;

class PingPongTask : public Task
{
	MSG_DECLARE_DYNAMIC(PingPongTask);

public:

	PingPongTask()
	:Task(NULL, 0.0, 0.0){}
	
	// Default constructor.
	PingPongTask(const char* name, double computeDuration, double messageSize)
	throw (InvalidArgumentException, NullPointerException)
	:Task(name, computeDuration, messageSize){}
	
	// Destructor
	virtual ~PingPongTask() {}
	
	void setTime(double timeVal)
	{
		this->timeVal = timeVal;
	}
	
	double getTime() 
	{
		return timeVal;
	}
	
private :
	
	double timeVal;
};


#endif // !PINGPONG_TASK_HPP