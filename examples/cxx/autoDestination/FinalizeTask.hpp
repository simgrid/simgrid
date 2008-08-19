#ifndef FINALIZE_TASK_HPP
#define FINALIZE_TASK_HPP

#include <Task.hpp>
using namespace SimGrid::Msg;

class FinalizeTask : public Task
{
	MSG_DECLARE_DYNAMIC(FinalizeTask);
public:
	
	// Default constructor.
	FinalizeTask()
	throw (InvalidArgumentException, NullPointerException)
	:Task("finalize", 0.0, 0.0){}
	
	// Destructor
	virtual ~FinalizeTask()
	throw (MsgException) {}
	
};

#endif // !FINALIZE_TASK_HPP
