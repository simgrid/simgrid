#ifndef SLAVE_HPP
#define SLAVE_HPP

#include <Process.hpp>
using namespace SimGrid::Msg;

class Slave : public Process
{
	MSG_DECLARE_DYNAMIC(Slave);

	public:
	
	// Default constructor.
	Slave(){}
	
	// Destructor.
	virtual ~Slave(){}
	
	int main(int argc, char** argv);
		
};

#endif // !SLAVE_HPP
