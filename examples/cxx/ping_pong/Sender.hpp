#ifndef SENDER_HPP
#define SENDER_HPP

#include <MsgProcess.hpp>
using namespace SimGrid::Msg;

class Sender : public Process
{
	MSG_DECLARE_DYNAMIC(Sender);

	public:
	
	// Default constructor.
	Sender(){}
	
	// Destructor.
	virtual ~Sender(){}
	
	int main(int argc, char** argv);
		
};


#endif // !SENDER_HPP

