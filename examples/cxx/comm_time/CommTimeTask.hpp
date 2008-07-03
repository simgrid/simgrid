#ifndef COMMTIME_TASK_HPP
#define COMMTIME_TASK_HPP

#include <Task.hpp>
using namespace SimGrid::Msg;

class CommTimeTask : public Task
{
	MSG_DECLARE_DYNAMIC(CommTimeTask);

public:

	CommTimeTask()
	:Task(NULL, 0.0, 0.0){}
	
	// Default constructor.
	CommTimeTask(const char* name, double computeDuration, double messageSize)
	throw (InvalidArgumentException, NullPointerException)
	:Task(name, computeDuration, messageSize){}
	
	// Destructor
	virtual ~CommTimeTask() {}
	
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




#endif // !COMMTIME_TASK_HPP