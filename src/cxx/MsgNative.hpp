#ifndef MSG_NATIVE_HPP
#define MSG_NATIVE_HPP

// Compilation C++ recquise
#ifndef __cplusplus
	#error MsgNative.hpp requires C++ compilation (use a .cxx suffix)
#endif

namespace msg
{
class MsgNative
{
	private:
	// Default constructor.
	MsgNative(){}
	
	// Copy constructor.
	MsgNative(const MsgNative& rMsgInterface) {}
	
	// Destructor.
	virtual ~MsgNative(){}
	
	public:
	
	
	static void processCreate(const Process& rProcess, const Host& rHost);
	
	static  int processKillAll(int resetPID);
	
	static void processSuspend(const Process& process);
	
	static void processKill(const Process& process);
	
	static  void processResume(const Process& process);
	
	static bool processIsSuspended(const Process& process);
	
	static Host& processGetHost(const Process& process);
	
	static Process& processFromPID(int PID);
	
	static int processGetPID(const Process& process);
	
	static int processGetPPID(const Process& process);
	
	static Process& processSelf(void);
	
	static int processSelfPID(void);
	
	static int processSelfPPID(void);
	
	static void processChangeHost(const Process& process, const Host& host);
	
	static void processWaitFor(double seconds);
	
	static void processExit(const Process& process);
	
	static Host& hostGetByName(const string& name) throw(HostNotFoundException);
	
	static string& hostGetName(const Host& host);
	
	static int hostGetNumber(void);
	
	static Host& hostSelf(void);
	
	static double hostGetSpeed(const Host& host);
	
	static bool hostIsAvail(const Host& host);
	
	static Host[] allHosts(void);
	
	static int hostGetLoad(const Host& host);
	
	static void taskCreate(const Task& task, const string& name, double computeDuration, double messageSize)
	throw(NullPointerException, IllegalArgumentException);
	
	static Process& taskGetSender(const Task& task);
	
	static Host& taskGetSource(const Task& task);
	
	static string& taskGetName(const Task& task);
	
	static void taskCancel(const Task& task);
	
	static void parallelTaskCreate(const Task& pTask, const string& name, Host[]hosts, double[]computeDurations, double[]messageSizes)
	throw(NullPointerException, IllegalArgumentException);
	
	static double taskGetComputeDuration(const Task& task);
	
	static double taskGetRemainingDuration(const Task& task);
	
	static void taskSetPriority(const Taska task, double priority);
	
	static void taskDestroy(const Task& task);
	
	static void taskExecute(const Task& task);
	
	static Task& taskGet(int channel, double timeout, const Host& host);
	
	static void taskSend(const string& alias, const Task& task, double timeout);
	
	static Task& taskReceive(const string& alias, double timeout, const Host& host);
	
	static int taskListenFrom(const string& alias);
	
	static bool taskListen(const string& alias);
	
	static int taskListenFromHost(const string& alias, const Host& host);
	
	static bool taskProbe(int channel);
	
	static int taskGetCommunicatingProcess(int channel);
	
	static int taskProbeHost(int channel, const Host& host);
	
	static void hostPut(const Host& host, int channel, const Task& task, double timeout);
	
	static void hostPutBounded(const Hos&t host, int channel, const Task& task, double max_rate);
	
	static void taskSendBounded(const string& alias, const Task& task, double maxrate);
};
}



#endif // !MSG_NATIVE_HPP 