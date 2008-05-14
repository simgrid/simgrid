#ifndef MSG_TASK_HPP
#define MSG_TASK_HPP

// Compilation C++ recquise
#ifndef __cplusplus
	#error Process.hpp requires C++ compilation (use a .cxx suffix)
#endif

namespace msg
{
class Task
{
	protected:
		// Default constructor.
		Task();
		
	public:
		// Copy constructor.
		Task(const Task& rTask);
		
		// Destructor.
		virtual ~Task()
		throw(NativeException);
		
	// Other constructors.
	Task(const char* name, double computeDuration, double messageSize);
	
	Task(const char* name, Host* hosts, double* computeDurations, double* messageSizes);
	
	
	// Getters/setters.
	const char* getName(void) const;
	
	
	Process& getSender(void) const;
	
	Host& getSource(void) const 
	throw(NativeException);
	
	double getComputeDuration(void) const;
	
	double getRemainingDuration(void) const;
	
	void setPriority(double priority);
	
	static Task& get(int channel) 
	throw(NativeException); 
	
	static Task& get(int channel, const Host& rHost) 
	throw(NativeException);
	
	static Task& get(int channel, double timeout, const Host& rHost) 
	throw(NativeException);  
	
	static bool probe(int channel);
	
	static int probe(int channel, const Host& rHost);
	
	void execute(void) 
	throw(NativeException);
	
	void cancel(void) 
	throw(NativeException);
	
	
	void send(void) 
	throw(NativeException);
	
	private:
		
		m_task_t nativeTask;
	
		
};

}

#endif // §MSG_TASK_HPP