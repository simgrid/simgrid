#ifndef __TYPES_H	
#define __TYPES_H

#include <def.h>

#include <portable.h>
#include <xbt/xbt_os_thread.h>
#include <xbt/strbuff.h>
#include <xbt.h>

#include <dirent.h>

#include <lstrings.h>
#include <vector.h>
#include <dictionary.h>

#ifdef _cplusplus
extern "C" {
#endif

/* 
 * byte type definition 
 */
#ifndef __BYTE_T_DEFINED
	typedef unsigned char byte;
	#define __BYTE_T_DEFINED
#endif

/*
 * file descriptor and pid types for portability.
 */

#ifdef WIN32
	
	#ifndef __FD_T_DEFINED
		typedef HANDLE fd_t;
		#define __FD_T_DEFINED
	#endif
	
	#ifndef __PID_T_DEFINED
		typedef HANDLE pid_t;
		#define __PID_T_DEFINED
	#endif
	
#else

	#ifndef __FD_T_DEFINED
		typedef int fd_t;
		#define __FD_T_DEFINED
	#endif
	
	#ifndef __PID_T_DEFINED
		typedef int pid_t;
		#define __PID_T_DEFINED
	#endif
#endif


/* forward declarations */
struct s_runner;
struct s_units;
struct s_unit;
struct s_suites;
struct s_suite;
struct s_excludes;
struct s_fstreams;
struct s_fstream;
struct s_directories;
struct s_directory;
struct s_writer;
struct s_reader;
struct s_timer;
struct s_context;


/* 
 * command status 
 */
typedef enum e_command_status
{
	cs_initialized					= 0,		/* the is initialized												*/
	cs_started						= 1,		/* the command is started											*/	
	cs_in_progress					= 2,		/* the command is execited											*/
	cs_waiting						= 3,		/* the command is waiting the writer, the reader and the timer		*/				
	cs_interrupted					= 4,		/* the command is interrupted										*/	
	cs_failed						= 5,		/* the command is failed											*/
	cs_successeded					= 6,		/* the command is successeded										*/
	cs_killed						= 7			/* the command is killed											*/
}command_status_t;

/* 
 * reason of the status of the command 
 */
typedef enum e_command_status_raison
{
	csr_unknown						= 0,		/* unknown reason													*/
	csr_read_failure				= 1,		/* a read operation failed											*/
	csr_read_pipe_broken			= 2,		/* the pipe used to read from the stdout of the command is broken	*/
	csr_timeout						= 3,		/* timeout															*/
	csr_write_failure				= 4,		/* a write operation failed											*/
	csr_write_pipe_broken			= 5,		/* the pipe used to write to the stdin of the command is broken		*/
	csr_exec_failure				= 6,		/* can't execute the command										*/
	csr_wait_failure				= 8,		/* the wait process function failed									*/
	csr_interruption_request		= 9,		/* the command has received an interruption request					*/
	csr_command_not_found			= 10,		/* the command is not found											*/
	csr_exit_codes_dont_match		= 11,
	csr_outputs_dont_match			= 12,
	csr_signals_dont_match			= 13,
	csr_unexpected_signal_caught	= 14,
	csr_expected_signal_not_receipt = 15,
	csr_pipe_function_failed		= 16		/* the function pipe() or CreatePipe() fails						*/
}cs_reason_t;


struct s_command;
struct s_variable;
struct s_variables;


/* 
 * declaration of the tesh timer type 
 */
typedef struct s_timer
{
	xbt_os_thread_t thread;				/* asynchronous timer													*/
	struct s_command* command;					/* the timed command													*/
	int timeouted;						/* if 1, the timer is timeouted											*/
	xbt_os_sem_t started;
}s_timer_t,* ttimer_t;

/* 
 * declaration of the tesh reader type 
 */
typedef struct s_reader
{
	xbt_os_thread_t thread;				/* asynchonous reader													*/
	struct s_command* command;					/* the command of the reader											*/
	int failed;							/* if 1, the reader failed												*/
	int broken_pipe;					/* if 1, the pipe used by the reader is broken							*/
	xbt_os_sem_t started;
}s_reader_t,* reader_t;

/* 
 * declaration of the tesh writer type 
 */
typedef struct s_writer
{
	xbt_os_thread_t thread;				/* asynchronous writer													*/
	struct s_command* command;					/* the command of the writer											*/
	int failed;							/* if 1, the writer failed												*/
	int broken_pipe;					/* if 1, the pipe used by the writer is broken							*/
	xbt_os_sem_t started;
}s_writer_t,* writer_t;

typedef struct s_units
{
	vector_t items;					/* used to store the units												*/
	int number_of_runned_units;		/* the number of units runned											*/
	int number_of_ended_units;		/* the number of units over												*/
	
}s_units_t,* units_t;

/* 
 * declaration of the tesh unit type 
 */
typedef struct s_unit
{
	struct s_fstream* fstream;
	struct s_runner* runner;					/* the runner of the unit											    */
	vector_t commands;
	int number_of_commands;				/* number of created commands of the unit								*/	
	int number_of_started_commands;		/* number of runned commands of the unit								*/
	int number_of_interrupted_commands;	/* number of interrupted commands of the unit							*/	
	int number_of_failed_commands;		/* number of failed commands of the unit								*/
	int number_of_successeded_commands;	/* number of successeded commands of the unit							*/
	int number_of_terminated_commands;	/* number of ended commands												*/
	xbt_os_thread_t thread;				/* all the units run in its own thread									*/
	xbt_os_sem_t sem;					/* used by the commands of the unit to signal the end of the unit		*/
	xbt_os_mutex_t mutex;				/* used to synchronously access to the properties of the runner			*/
	int interrupted;					/* if 1, the unit is interrupted by the runner							*/
	int failed;							/* if 1, the unit is failed												*/
	int successeded;					/* if 1, the unit is successeded										*/
	int parsed;							/* if 1, the tesh file of the unit is parsed							*/
	int released;
	int parsing_include_file;
	struct s_suite* owner;						/* the suite containing the unit if any									*/
	vector_t suites;
	int number;							/* the number of suites													*/
	int capacity;						/* the number of suites that the unit can contain						*/
	int running_suite;					/* if 1, the suite running a suite										*/									
}s_unit_t,* unit_t;


/* 
 * declaration of tesh suite type
 */
typedef struct s_suite
{
	const char* description;			/* the name of the suite												*/
	struct s_unit* owner;						/* the unit owning the suite											*/
	vector_t units;						/* the units of the suite												*/		
	int number;							/* the numnber of suites contained by the suite 						*/			
	int capacity;						/* the number of suites that the vector can contain 					*/
	int number_of_successed_units;		/* the number of successeded units of the suite							*/
	int number_of_failed_units;			/* the number of failed units of the suite								*/	
}s_suite_t,* suite_t;

/* 
 * declaration of the tesh runner type 
 */
typedef struct s_runner
{
	
	/*vector_t units;*/						/* the vector containing all the units launched by the runner			*/
	struct s_units* units;
	int timeouted;						/* if 1, the runner is timeouted										*/
	int timeout;						/* the timeout of the runner											*/
	int interrupted;					/* if 1, the runner failed												*/
	int number_of_runned_units;			/* the number of units runned by the runner								*/
	int number_of_ended_units;			/* the number of ended units											*/
	int waiting;						/* if 1, the runner is waiting the end of all the units					*/
	xbt_os_thread_t thread;				/* the timer thread														*/
}s_runner_t,* runner_t;


typedef struct s_fstreams
{
	vector_t items;
}s_fstreams_t,* fstreams_t;


typedef struct s_fstream
{
	char* name;
	char* directory;
	FILE* stream;
}s_fstream_t,* fstream_t;

typedef struct s_excludes
{
	vector_t items;
}s_excludes_t,* excludes_t;

typedef struct s_directory
{
	char* name;
	DIR* stream;
	int load;
}s_directory_t,* directory_t;

typedef struct s_directories
{
	vector_t items;
}s_directories_t,* directories_t;


typedef enum 
{
	oh_check, 
	oh_display, 
	oh_ignore
}output_handling_t;
/* 
 * declaration of the tesh context type 
 */
typedef struct s_context
{
	const char* command_line;			/* the command line of the command to execute							*/
	const char* line;					/* the current parsed line												*/
	int exit_code;						/* the expected exit code of the command								*/
	char* signal;						/* the expected signal raised by the command							*/
	int timeout;						/* the timeout of the test												*/
	xbt_strbuff_t input;				/* the input to write in the stdin of the command to run				*/
	xbt_strbuff_t output;				/* the expected output of the command of the test						*/
	output_handling_t output_handling;
	int async;							/* if 1, the command is asynchronous									*/
}s_context_t,* context_t;

/* 
 * declaration of the tesh command type 
 */
typedef struct s_command
{
	unit_t unit;						/* the unit of the command												*/
	struct s_context* context;					/* the context of the execution of the command							*/
	xbt_os_thread_t thread;				/* asynchronous command													*/
	struct s_writer* writer;					/* the writer used to write in the command stdin						*/
	struct s_reader* reader;					/* the reader used to read from the command stout						*/
	struct s_timer* timer;						/* the timer used for the command										*/
	command_status_t status;			/* the current status of the command									*/
	cs_reason_t reason;					/* the reason of the state of the command								*/
	int successeded;					/* if 1, the command is successeded										*/
	int interrupted;					/* if 1, the command is interrupted										*/
	int failed;							/* if 1, the command is failed											*/
	pid_t pid;							/* the program id of the command										*/
	xbt_strbuff_t output;				/* the output of the command											*/
	fd_t stdout_fd;						/* the stdout fd of the command											*/
	fd_t stdin_fd;						/* the stdin fd of the command											*/
	int exit_code;						/* the exit code of the command											*/
	#ifdef WIN32
	unsigned long stat_val;
	#else
	int stat_val;
	#endif
	char* signal;						/* the signal raised by the command if any								*/
	xbt_os_mutex_t mutex;
	#ifndef WIN32
	int killed;							/* if 1, the command was killed											*/
	#endif
}s_command_t,* command_t;

#ifdef _cplusplus
}
#endif


#endif /* !__TYPES_H */
