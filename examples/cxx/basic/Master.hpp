#ifndef MASTER_HPP
#define MASTER_HPP

#include <Process.hpp>
using namespace SimGrid::Msg;

class Master : public Process
{
	MSG_DECLARE_DYNAMIC(Master);

	public:
	
	// Default constructor.
	Master(){}
	
	// Destructor.
	virtual ~Master(){}
	
	int main(int argc, char** argv);
		
};

#endif // !MASTER_HPP