#ifndef MSG_PROCESS_HPP
#define MSG_PROCESS_HPP

// Compilation C++ recquise
#ifndef __cplusplus
	#error Process.hpp requires C++ compilation (use a .cxx suffix)
#endif

namespace msg
{
	
class Process
{
	
	friend ApplicationHandler;
	
	private;
	
	// Default constructor.
	Process();
	
	public:
	
	// Construct a process from the name of the host and its name.	
	Process(const char* hostName, const char* name)
	throw(HostNotFoundException);
	
	Process(const Host& rHost, const char* name)
	throw(HostNotFoundException);
	
	Process(const char* hostName, const char* name, int argc, char** argv)
	throw(HostNotFoundException);
	
	Process(const Host& rHost, const char* name, int argc, char** argv)
	throw(HostNotFoundException);
	
	static int killAll(int resetPID);
	
	void Process::suspend(void)
	throw(NativeException);
	
	void Process::resume(void) 
	throw(NativeException);
	
	bool isSuspended(void);
	
	Host& getHost(void) 
	throw(NativeException);
	
	static Process& fromPID(int PID); 
	
	
	int getPID(void);
		
	int getPPID(void);
	
	const char* getName(void) const;
	
	static Process& currentProcess(void);
	
	static int currentProcessPID(void);
	
	static int currentProcessPPID(void);
	
	void migrate(const Host& rHost)
	throw(NativeException);
	
	static void sleep(double seconds)
	throw(NativeException);
	
	void putTask(const Host& rHost, int channel, const Task& rTask)
	throw( NativeException);
	
	void putTask(const Host& rHost, int channel, const Task& rTask, double timeout) 
	throw(NativeException);
	
	Task& getTask(int channel) 
	throw(NativeException);
	
	Task& getTask(int channel, double timeout) 
	throw(NativeException);
	
	Task& getTask(int channel, const Host& rHost) 
	throw(NativeException);
	
	Task& getTask(int channel, double timeout, const Host& rHost)
	throw(NativeException);
	
	void sendTask(const char* alias, const Task& rTask, double timeout) 
	throw(NativeException);
	
	void sendTask(const char* alias, const Task& rTask) 
	throw(NativeException);
	
	void sendTask(const Task& rTask) 
	throw(NativeException);
	
	void sendTask(const Task& rTask, double timeout) 
	throw(NativeException);
	
	Task& receiveTask(const char* alias) 
	throw(NativeException);
	
	Task& receiveTask(void) 
	throw(NativeException);
	
	Task& receiveTask(const char* alias, double timeout) 
	throw(NativeException);
	
	Task& receiveTask(double timeout) 
	throw(NativeException);
	
	Task& receiveTask(const char* alias, double timeout, const Host& rHost) 
	throw(NativeException);
	
	Task& Process::receiveTask(double timeout, const Host& rHost) 
	throw(NativeException);
	
	Task& receiveTask(const char* alias, const Host& rHost) 
	throw(NativeException);
	
	Task& receiveTask(const Host& rHost) 
	throw(NativeException);
	
		
	private:
	void create(const Host& rHost, const char* name, int argc, char** argv) 
	throw(HostNotFoundException);
	
	
	static Process& fromNativeProcess(m_process_t nativeProcess);
	
	
	public:
		
	static int run(int argc, char** argv);
	
	virtual int main(int argc, char** argv) = 0;
		
	private:
		
	// Attributes.
	
	m_process_t nativeProcess;	// pointer to the native msg process.
	
};

}