/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_SMURF_PRIVATE_H
#define _SIMIX_SMURF_PRIVATE_H

/********************************* Simcalls *********************************/

/* we want to build the e_smx_simcall_t enumeration, the table of the
 * corresponding simcalls string names, and the simcall handlers table
 * automatically, using macros.
 * To add a new simcall follow the following syntax:
 *
 * SIMCALL_ENUM_ELEMENT(<simcall_enumeration_id>, <simcall_handler_function>)
 *
 * */

/****************************
 * SIMCALL GENERATING MACRO *
 ****************************
 *
 * action(ENUM_NAME, func_name, result_type, params…) 
 *
 **/

/*
 * Some macro machinery to get a MAP over the arguments of a variadic macro.
 * It uses a FOLD to apply a macro to every argument, and because there is
 * no recursion in the C preprocessor we must create a new macro for every
 * depth of FOLD's recursion.
 */

/* FOLD macro */
#define FE_0(WHAT, X, ...)
#define FE_1(I, WHAT, X) WHAT(I, X)
#define FE_2(I, WHAT, X, ...) WHAT(I, X), FE_1(I+1, WHAT, __VA_ARGS__)
#define FE_3(I, WHAT, X, ...) WHAT(I, X), FE_2(I+1, WHAT, __VA_ARGS__)
#define FE_4(I, WHAT, X, ...) WHAT(I, X), FE_3(I+1, WHAT, __VA_ARGS__)
#define FE_5(I, WHAT, X, ...) WHAT(I, X), FE_4(I+1, WHAT, __VA_ARGS__)
#define FE_6(I, WHAT, X, ...) WHAT(I, X), FE_5(I+1, WHAT, __VA_ARGS__)
#define FE_7(I, WHAT, X, ...) WHAT(I, X), FE_6(I+1, WHAT, __VA_ARGS__)
#define FE_8(I, WHAT, X, ...) WHAT(I, X), FE_7(I+1, WHAT, __VA_ARGS__)
#define FE_9(I, WHAT, X, ...) WHAT(I, X), FE_8(I+1, WHAT, __VA_ARGS__)
#define FE_10(I, WHAT, X, ...) WHAT(I, X), FE_9(I+1, WHAT, __VA_ARGS__)


/* NOTE: add as many FE_n as needed (maximum number of simcall arguments )*/

/* Make a MAP macro usgin FOLD (will apply 'action' to the arguments.
 * GET_MACRO is a smart hack that counts the number of arguments passed to
 * the variadic macro, and it is used to invoke the right FOLD depth.
 */
#define GET_MACRO(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,NAME,...) NAME
#define MAP(action, ...) \
  GET_MACRO(, ##__VA_ARGS__, FE_10,FE_9,FE_8,FE_7,FE_6,FE_5,FE_4,FE_3,FE_2,FE_1, FE_0) (0, action, __VA_ARGS__)

/*
 * Define scalar type wrappers to ease the use of simcalls.
 * These are used to wrap the arguments in SIMIX_simcall macro.
 */
#define TCHAR(n) (n, char, c)
#define TSTRING(n) (n, const char*, cc)
#define TSHORT(n) (n, short, s)
#define TINT(n) (n, int, i)
#define TLONG(n) (n, long, l)
#define TUCHAR(n) (n, unsigned char, uc)
#define TUSHORT(n) (n, unsigned short, us)
#define TUINT(n) (n, unsigned int, ui)
#define TULONG(n) (n, unsigned long, ul)
#define TFLOAT(n) (n, float, f)
#define TDOUBLE(n) (n, double, d)
#define TPTR(n) (n, void*, p)
#define TCPTR(n) (n, const void*, cp)
#define TSIZE(n) (n, size_t, si)
#define TSTAT(n) (n, s_file_stat_t, fs)
#define TVOID(n) (n, void)
#define TSPEC(n,t) (n, t, p)

/* use comma or nothing to separate elements*/
#define SIMCALL_SEP_COMMA ,
#define SIMCALL_SEP_NOTHING

/* get the name of the parameter */
#define SIMCALL_NAME_(name, type, field) name
#define SIMCALL_NAME(i, v) SIMCALL_NAME_ v

/* get the %s format code of the parameter */
#define SIMCALL_FORMAT_(name, type, field) %field
#define SIMCALL_FORMAT(i, v) SIMCALL_FORMAT_ v

/* get the parameter declaration */
#define SIMCALL_ARG_(name, type, field) type name
#define SIMCALL_ARG(i, v) SIMCALL_ARG_ v

/* get the parameter initialisation field */
#define SIMCALL_INIT_FIELD_(name, type, field) {.field = name}
#define SIMCALL_INIT_FIELD(i, v) SIMCALL_INIT_FIELD_ v

/* get the case of the parameter */
#define SIMCALL_CASE_PARAM_(name, type, field) field
#define SIMCALL_CASE_PARAM(i, v) simcall->args[i].SIMCALL_CASE_PARAM_ v

/* generate some code for SIMCALL_CASE if the simcall has an answer */
#define MAYBE2(_0, _1, func, ...) func

#define SIMCALL_WITH_RESULT_BEGIN(name, type, field) simcall->result.field =
#define SIMCALL_WITHOUT_RESULT_BEGIN(name, type, field)
#define SIMCALL_RESULT_BEGIN_(name, type, ...)\
        MAYBE2(,##__VA_ARGS__, SIMCALL_WITH_RESULT_BEGIN, SIMCALL_WITHOUT_RESULT_BEGIN)\
	(name, type, __VA_ARGS__)
#define SIMCALL_RESULT_BEGIN(answer, res) answer(SIMCALL_RESULT_BEGIN_ res)

#define SIMCALL_RESULT_END_(name, type, ...)\
	SIMIX_simcall_answer(simcall);
#define SIMCALL_RESULT_END(answer, res) answer(SIMCALL_RESULT_END_ res)

/* generate some code for BODY function */
#define SIMCALL_FUNC_RETURN_TYPE_(name, type, ...) type
#define SIMCALL_FUNC_RETURN_TYPE(res) SIMCALL_FUNC_RETURN_TYPE_ res

#define SIMCALL_WITH_FUNC_SIMCALL(name, type, field) smx_simcall_t simcall = 
#define SIMCALL_WITHOUT_FUNC_SIMCALL(name, type, field)
#define SIMCALL_FUNC_SIMCALL_(name, type, ...)\
        MAYBE2(,##__VA_ARGS__, SIMCALL_WITH_FUNC_SIMCALL, SIMCALL_WITHOUT_FUNC_SIMCALL)\
	(name, type, __VA_ARGS__)
#define SIMCALL_FUNC_SIMCALL(res) SIMCALL_FUNC_SIMCALL_ res

#define SIMCALL_WITH_FUNC_RETURN(name, type, field) return simcall->result.field;
#define SIMCALL_WITHOUT_FUNC_RETURN(name, type, field)
#define SIMCALL_FUNC_RETURN_(name, type, ...)\
        MAYBE2(,##__VA_ARGS__, SIMCALL_WITH_FUNC_RETURN, SIMCALL_WITHOUT_FUNC_RETURN)\
	(name, type, __VA_ARGS__)
#define SIMCALL_FUNC_RETURN(res) SIMCALL_FUNC_RETURN_ res


/* generate the simcall enumeration */
#define SIMCALL_ENUM(type, ...)\
	type

/* generate strings from the enumeration values */
#define SIMCALL_TYPE(type, name, answer, res, ...)\
	[type] = STRINGIFY(MAP(SIMCALL_FORMAT, __VA_ARGS__))

/* generate the simcalls functions */
#define SIMCALL_FUNC(type, name, answer, res, ...)\
	SIMCALL_FUNC_RETURN_TYPE(res) simcall_BODY_##name(MAP(SIMCALL_ARG, ##__VA_ARGS__)) { \
      SIMCALL_FUNC_SIMCALL(res) __SIMIX_simcall(type, (u_smx_scalar_t[]){MAP(SIMCALL_INIT_FIELD, ##__VA_ARGS__)}); \
      SIMCALL_FUNC_RETURN(res)\
    }

/* generate the simcalls prototypes functions */
#define VOID_IF_EMPTY(...) GET_CLEAN(,##__VA_ARGS__,,,,,,,,,,,void)
#define SIMCALL_FUNC_PROTO(type, name, answer, res, ...)\
	SIMCALL_FUNC_RETURN_TYPE(res) simcall_BODY_##name(VOID_IF_EMPTY(__VA_ARGS__) MAP(SIMCALL_ARG, ##__VA_ARGS__));


/* generate a comma if there is an argument*/
#define WITHOUT_COMMA 
#define WITH_COMMA ,
#define GET_CLEAN(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10, NAME,...) NAME
#define MAYBE_COMMA(...) GET_CLEAN(,##__VA_ARGS__,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITHOUT_COMMA)

/* generate the simcalls cases for the SIMIX_simcall_pre function */
#define WITH_ANSWER(...) __VA_ARGS__
#define WITHOUT_ANSWER(...) 
#define SIMCALL_CASE(type, name, answer, res, ...)\
    case type:;\
      SIMCALL_RESULT_BEGIN(answer, res) SIMIX_pre_ ## name(simcall MAYBE_COMMA(__VA_ARGS__) MAP(SIMCALL_CASE_PARAM, ##__VA_ARGS__));\
      SIMCALL_RESULT_END(answer, res)\
      break;

/* stringify arguments */
#define STRINGIFY_(...) #__VA_ARGS__
#define STRINGIFY(...) STRINGIFY_(__VA_ARGS__)

/* the list of simcalls definitions */
#define SIMCALL_LIST1(action, sep) \
action(SIMCALL_HOST_GET_BY_NAME, host_get_by_name, WITH_ANSWER, TSPEC(result, smx_host_t), TSTRING(NAME)) sep \
action(SIMCALL_HOST_GET_NAME, host_get_name, WITH_ANSWER, TSTRING(result), TSPEC(host, smx_host_t)) sep \
action(SIMCALL_HOST_GET_PROPERTIES, host_get_properties, WITH_ANSWER, TSPEC(result, xbt_dict_t), TSPEC(host, smx_host_t)) sep \
action(SIMCALL_HOST_GET_SPEED, host_get_speed, WITH_ANSWER, TDOUBLE(result), TSPEC(host, smx_host_t)) sep \
action(SIMCALL_HOST_GET_AVAILABLE_SPEED, host_get_available_speed, WITH_ANSWER, TDOUBLE(result), TSPEC(result, smx_host_t)) sep \
action(SIMCALL_HOST_GET_STATE, host_get_state, WITH_ANSWER, TINT(result), TSPEC(host, smx_host_t)) sep \
action(SIMCALL_HOST_GET_DATA, host_get_data, WITH_ANSWER, TPTR(result), TSPEC(host, smx_host_t)) sep \
action(SIMCALL_HOST_SET_DATA, host_set_data, WITH_ANSWER, TVOID(result), TSPEC(host, smx_host_t), TPTR(data)) sep \
action(SIMCALL_HOST_EXECUTE, host_execute, WITH_ANSWER, TSPEC(result, smx_action_t), TSTRING(name), TSPEC(host, smx_host_t), TDOUBLE(computation_amount), TDOUBLE(priority)) sep \
action(SIMCALL_HOST_PARALLEL_EXECUTE, host_parallel_execute, WITH_ANSWER, TSPEC(result, smx_action_t), TSTRING(name), TINT(host_nb), TSPEC(host_list, smx_host_t*), TSPEC(computation_amount, double*), TSPEC(communication_amount, double*), TDOUBLE(amount), TDOUBLE(rate)) sep \
action(SIMCALL_HOST_EXECUTION_DESTROY, host_execution_destroy, WITH_ANSWER, TVOID(result), TSPEC(execution, smx_action_t)) sep \
action(SIMCALL_HOST_EXECUTION_CANCEL, host_execution_cancel, WITH_ANSWER, TVOID(result), TSPEC(execution, smx_action_t)) sep \
action(SIMCALL_HOST_EXECUTION_GET_REMAINS, host_execution_get_remains, WITH_ANSWER, TDOUBLE(result), TSPEC(execution, smx_action_t)) sep \
action(SIMCALL_HOST_EXECUTION_GET_STATE, host_execution_get_state, WITH_ANSWER, TINT(result), TSPEC(execution, smx_action_t)) sep \
action(SIMCALL_HOST_EXECUTION_SET_PRIORITY, host_execution_set_priority, WITH_ANSWER, TVOID(result), TSPEC(execution, smx_action_t), TDOUBLE(priority)) sep \
action(SIMCALL_HOST_EXECUTION_WAIT, host_execution_wait, WITHOUT_ANSWER, TINT(result), TSPEC(execution, smx_action_t)) sep \
action(SIMCALL_PROCESS_CREATE, process_create, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t*), TSTRING(name), TSPEC(code, xbt_main_func_t), TPTR(data), TSTRING(hostname), TDOUBLE(kill_time), TINT(argc), TSPEC(argv, char**), TSPEC(properties, xbt_dict_t), TINT(auto_restart)) sep \
action(SIMCALL_PROCESS_KILL, process_kill, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t)) sep \
action(SIMCALL_PROCESS_KILLALL, process_killall, WITH_ANSWER, TVOID(result)) sep \
action(SIMCALL_PROCESS_CLEANUP, process_cleanup, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t)) sep \
action(SIMCALL_PROCESS_CHANGE_HOST, process_change_host, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t), TSPEC(dest, smx_host_t)) sep \
action(SIMCALL_PROCESS_SUSPEND, process_suspend, WITHOUT_ANSWER, TVOID(result), TSPEC(process, smx_process_t)) sep \
action(SIMCALL_PROCESS_RESUME, process_resume, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t)) sep \
action(SIMCALL_PROCESS_COUNT, process_count, WITH_ANSWER, TINT(result)) sep \
action(SIMCALL_PROCESS_GET_DATA, process_get_data, WITH_ANSWER, TPTR(result), TSPEC(process, smx_process_t)) sep \
action(SIMCALL_PROCESS_SET_DATA, process_set_data, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t), TPTR(data)) sep \
action(SIMCALL_PROCESS_GET_HOST, process_get_host, WITH_ANSWER, TSPEC(result, smx_host_t), TSPEC(process, smx_process_t)) sep \
action(SIMCALL_PROCESS_GET_NAME, process_get_name, WITH_ANSWER, TSTRING(result), TSPEC(process, smx_process_t)) sep \
action(SIMCALL_PROCESS_IS_SUSPENDED, process_is_suspended, WITH_ANSWER, TINT(result), TSPEC(process, smx_process_t)) sep \
action(SIMCALL_PROCESS_GET_PROPERTIES, process_get_properties, WITH_ANSWER, TSPEC(result, xbt_dict_t), TSPEC(process, smx_process_t)) sep \
action(SIMCALL_PROCESS_SLEEP, process_sleep, WITHOUT_ANSWER, TINT(result), TDOUBLE(duration)) sep \
action(SIMCALL_PROCESS_ON_EXIT, process_on_exit, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t), TSPEC(fun, int_f_pvoid_t), TPTR(data)) sep \
action(SIMCALL_PROCESS_AUTO_RESTART_SET, process_auto_restart_set, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t), TINT(auto_restart)) sep \
action(SIMCALL_PROCESS_RESTART, process_restart, WITH_ANSWER, TSPEC(result, smx_process_t), TSPEC(process, smx_process_t)) sep \
action(SIMCALL_RDV_CREATE, rdv_create, WITH_ANSWER, TSPEC(result, smx_rdv_t), TSTRING(name)) sep \
action(SIMCALL_RDV_DESTROY, rdv_destroy, WITH_ANSWER, TVOID(result), TSPEC(rdv, smx_rdv_t)) sep \
action(SIMCALL_RDV_GET_BY_NAME, rdv_get_by_name, WITH_ANSWER, TSPEC(result, smx_host_t), TSTRING(name)) sep \
action(SIMCALL_RDV_COMM_COUNT_BY_HOST, rdv_comm_count_by_host, WITH_ANSWER, TUINT(result), TSPEC(rdv, smx_rdv_t), TSPEC(host, smx_host_t)) sep \
action(SIMCALL_RDV_GET_HEAD, rdv_get_head, WITH_ANSWER, TSPEC(result, smx_action_t), TSPEC(rdv, smx_rdv_t)) sep \
action(SIMCALL_RDV_SET_RECV, rdv_set_receiver, WITH_ANSWER, TVOID(result), TSPEC(rdv, smx_rdv_t), TSPEC(receiver, smx_process_t)) sep \
action(SIMCALL_RDV_GET_RECV, rdv_get_receiver, WITH_ANSWER, TSPEC(result, smx_process_t), TSPEC(rdv, smx_rdv_t)) sep \
action(SIMCALL_COMM_IPROBE, comm_iprobe, WITH_ANSWER, TSPEC(result, smx_action_t), TSPEC(rdv, smx_rdv_t), TINT(src), TINT(tag), TSPEC(match_fun, simix_match_func_t), TPTR(data)) sep \
action(SIMCALL_COMM_SEND, comm_send, WITHOUT_ANSWER, TVOID(result), TSPEC(rdv, smx_rdv_t), TDOUBLE(task_size), TDOUBLE(rate), TPTR(src_buff), TSIZE(src_buff_size), TSPEC(match_fun, simix_match_func_t), TPTR(data), TDOUBLE(timeout)) sep \
action(SIMCALL_COMM_ISEND, comm_isend, WITH_ANSWER, TSPEC(result, smx_action_t), TSPEC(rdv, smx_rdv_t), TDOUBLE(task_size), TDOUBLE(rate), TPTR(src_buff), TSIZE(src_buff_size), TSPEC(match_fun, simix_match_func_t), TSPEC(clean_fun, simix_clean_func_t), TPTR(data), TINT(detached)) sep \
action(SIMCALL_COMM_RECV, comm_recv, WITHOUT_ANSWER, TVOID(result), TSPEC(rdv, smx_rdv_t), TPTR(dst_buff), TSPEC(dst_buff_size, size_t*), TSPEC(match_fun, simix_match_func_t), TPTR(data), TDOUBLE(timeout)) sep \
action(SIMCALL_COMM_IRECV, comm_irecv, WITH_ANSWER, TSPEC(result, smx_action_t), TSPEC(rdv, smx_rdv_t), TPTR(dst_buff), TSPEC(dst_buff_size, size_t*), TSPEC(match_fun, simix_match_func_t), TPTR(data)) sep \
action(SIMCALL_COMM_DESTROY, comm_destroy, WITH_ANSWER, TVOID(result), TSPEC(comm, smx_action_t)) sep \
action(SIMCALL_COMM_CANCEL, comm_cancel, WITH_ANSWER, TVOID(result), TSPEC(comm, smx_action_t)) sep \
action(SIMCALL_COMM_WAITANY, comm_waitany, WITHOUT_ANSWER, TINT(result), TSPEC(comms, xbt_dynar_t)) sep \
action(SIMCALL_COMM_WAIT, comm_wait, WITHOUT_ANSWER, TVOID(result), TSPEC(comm, smx_action_t), TDOUBLE(timeout)) sep \
action(SIMCALL_COMM_TEST, comm_test, WITHOUT_ANSWER, TINT(result), TSPEC(comm, smx_action_t)) sep \
action(SIMCALL_COMM_TESTANY, comm_testany, WITHOUT_ANSWER, TINT(result), TSPEC(comms, xbt_dynar_t)) sep \
action(SIMCALL_COMM_GET_REMAINS, comm_get_remains, WITH_ANSWER, TDOUBLE(result), TSPEC(comm, smx_action_t)) sep \
action(SIMCALL_COMM_GET_STATE, comm_get_state, WITH_ANSWER, TINT(result), TSPEC(comm, smx_action_t)) sep \
action(SIMCALL_COMM_GET_SRC_DATA, comm_get_src_data, WITH_ANSWER, TPTR(result), TSPEC(comm, smx_action_t)) sep \
action(SIMCALL_COMM_GET_DST_DATA, comm_get_dst_data, WITH_ANSWER, TPTR(result), TSPEC(comm, smx_action_t)) sep \
action(SIMCALL_COMM_GET_SRC_PROC, comm_get_src_proc, WITH_ANSWER, TSPEC(result, smx_process_t), TSPEC(comm, smx_action_t)) sep \
action(SIMCALL_COMM_GET_DST_PROC, comm_get_dst_proc, WITH_ANSWER, TSPEC(result, smx_process_t), TSPEC(comm, smx_action_t)) sep \
action(SIMCALL_MUTEX_INIT, mutex_init, WITH_ANSWER, TSPEC(result, smx_mutex_t)) sep \
action(SIMCALL_MUTEX_DESTROY, mutex_destroy, WITH_ANSWER, TVOID(result), TSPEC(mutex, smx_mutex_t)) sep \
action(SIMCALL_MUTEX_LOCK, mutex_lock, WITHOUT_ANSWER, TVOID(result), TSPEC(mutex, smx_mutex_t)) sep \
action(SIMCALL_MUTEX_TRYLOCK, mutex_trylock, WITH_ANSWER, TINT(result), TSPEC(mutex, smx_mutex_t)) sep \
action(SIMCALL_MUTEX_UNLOCK, mutex_unlock, WITH_ANSWER, TVOID(result), TSPEC(mutex, smx_mutex_t)) sep \
action(SIMCALL_COND_INIT, cond_init, WITH_ANSWER, TSPEC(result, smx_cond_t)) sep \
action(SIMCALL_COND_DESTROY, cond_destroy, WITH_ANSWER, TVOID(result), TSPEC(cond, smx_cond_t)) sep \
action(SIMCALL_COND_SIGNAL, cond_signal, WITH_ANSWER, TVOID(result), TSPEC(cond, smx_cond_t)) sep \
action(SIMCALL_COND_WAIT, cond_wait, WITHOUT_ANSWER, TVOID(result), TSPEC(cond, smx_cond_t), TSPEC(mutex, smx_mutex_t)) sep \
action(SIMCALL_COND_WAIT_TIMEOUT, cond_wait_timeout, WITHOUT_ANSWER, TVOID(result), TSPEC(cond, smx_cond_t), TSPEC(mutex, smx_mutex_t), TDOUBLE(timeout)) sep \
action(SIMCALL_COND_BROADCAST, cond_broadcast, WITH_ANSWER, TVOID(result), TSPEC(cond, smx_cond_t)) sep \
action(SIMCALL_SEM_INIT, sem_init, WITH_ANSWER, TSPEC(result, smx_sem_t), TINT(capacity)) sep \
action(SIMCALL_SEM_DESTROY, sem_destroy, WITH_ANSWER, TVOID(result), TSPEC(sem, smx_sem_t)) sep \
action(SIMCALL_SEM_RELEASE, sem_release, WITH_ANSWER, TVOID(result), TSPEC(sem, smx_sem_t)) sep \
action(SIMCALL_SEM_WOULD_BLOCK, sem_would_block, WITH_ANSWER, TINT(result), TSPEC(sem, smx_sem_t)) sep \
action(SIMCALL_SEM_ACQUIRE, sem_acquire, WITHOUT_ANSWER, TVOID(result), TSPEC(sem, smx_sem_t)) sep \
action(SIMCALL_SEM_ACQUIRE_TIMEOUT, sem_acquire_timeout, WITHOUT_ANSWER, TVOID(result), TSPEC(sem, smx_sem_t), TDOUBLE(timeout)) sep \
action(SIMCALL_SEM_GET_CAPACITY, sem_get_capacity, WITH_ANSWER, TINT(result), TSPEC(sem, smx_sem_t)) sep \
action(SIMCALL_FILE_READ, file_read, WITHOUT_ANSWER, TDOUBLE(result), TPTR(ptr), TSIZE(size), TSIZE(nmemb), TSPEC(stream, smx_file_t)) sep \
action(SIMCALL_FILE_WRITE, file_write, WITHOUT_ANSWER, TSIZE(result), TCPTR(ptr), TSIZE(size), TSIZE(nmemb), TSPEC(stream, smx_file_t)) sep \
action(SIMCALL_FILE_OPEN, file_open, WITHOUT_ANSWER, TSPEC(result, smx_file_t), TSTRING(mount), TSTRING(path), TSTRING(mode)) sep \
action(SIMCALL_FILE_CLOSE, file_close, WITHOUT_ANSWER, TINT(result), TSPEC(fp, smx_file_t)) sep \
action(SIMCALL_FILE_STAT, file_stat, WITHOUT_ANSWER, TINT(result), TSPEC(fd, smx_file_t), TSTAT(buf)) sep \
action(SIMCALL_FILE_UNLINK, file_unlink, WITHOUT_ANSWER, TINT(result), TSPEC(fd, smx_file_t)) sep \
action(SIMCALL_FILE_LS, file_ls, WITHOUT_ANSWER, TSPEC(result, xbt_dict_t), TSTRING(mount), TSTRING(path)) sep \
action(SIMCALL_ASR_GET_PROPERTIES, asr_get_properties, WITH_ANSWER, TSPEC(result, xbt_dict_t), TSTRING(name)) sep 

/* SIMCALL_COMM_IS_LATENCY_BOUNDED and SIMCALL_SET_CATEGORY make things complicated
 * because they are not always present */
#ifdef HAVE_LATENCY_BOUND_TRACKING
#define SIMCALL_LIST2(action, sep) \
action(SIMCALL_COMM_IS_LATENCY_BOUNDED, comm_is_latency_bounded, WITH_ANSWER, TINT(result), TSPEC(comm, smx_action_t)) sep
#else
#define SIMCALL_LIST2(action, sep)
#endif

#ifdef HAVE_TRACING
#define SIMCALL_LIST3(action, sep) \
action(SIMCALL_SET_CATEGORY, set_category, WITH_ANSWER, TVOID(result), TSPEC(action, smx_action_t), TSTRING(category)) sep
#else
#define SIMCALL_LIST3(action, sep)
#endif

#ifdef HAVE_MC
#define SIMCALL_LIST4(action, sep) \
action(SIMCALL_MC_SNAPSHOT, mc_snapshot, WITH_ANSWER, TPTR(result)) sep \
action(SIMCALL_MC_COMPARE_SNAPSHOTS, mc_compare_snapshots, WITH_ANSWER, TINT(result), TPTR(s1), TPTR(s2)) sep 
#else
#define SIMCALL_LIST4(action, sep)
#endif

/* SIMCALL_LIST is the final macro to use */
#define SIMCALL_LIST(action, ...) \
  SIMCALL_LIST1(action, ##__VA_ARGS__)\
  SIMCALL_LIST2(action, ##__VA_ARGS__)\
  SIMCALL_LIST3(action, ##__VA_ARGS__)\
  SIMCALL_LIST4(action, ##__VA_ARGS__)


/* you can redefine the following macro differently to generate something else
 * with the list of enumeration values (e.g. a table of strings or a table of function pointers) */
#define SIMCALL_ENUM_ELEMENT(x, y) x

/**
 * \brief All possible simcalls.
 */
typedef enum {
SIMCALL_NONE,
SIMCALL_LIST(SIMCALL_ENUM, SIMCALL_SEP_COMMA)
SIMCALL_NEW_API_INIT,
NUM_SIMCALLS
} e_smx_simcall_t;

typedef int (*simix_match_func_t)(void *, void *, smx_action_t);
typedef void (*simix_clean_func_t)(void *);

/**
 * \brief Prototypes of SIMIX
 */
SIMCALL_LIST(SIMCALL_FUNC_PROTO, SIMCALL_SEP_NOTHING)


/* Pack all possible scalar types in an union */
union u_smx_scalar {
  char            c;
  const char*     cc;
  short           s;
  int             i;
  long            l;
  unsigned char   uc;
  unsigned short  us;
  unsigned int    ui;
  unsigned long   ul;
  float           f;
  double          d;
  size_t          si;
  s_file_stat_t   fs;
  void*           p;
  const void*     cp;
};

/**
 * \brief Represents a simcall to the kernel.
 */
typedef struct s_smx_simcall {
  e_smx_simcall_t call;
  smx_process_t issuer;
  int mc_value;
  union u_smx_scalar *args;
  union u_smx_scalar result;
  //FIXME: union u_smx_scalar retval;

  union {

    struct {
      const char *name;
      smx_host_t result;
    } host_get_by_name;

    struct {
      smx_host_t host;
      const char* result;
    } host_get_name;

    struct {
      smx_host_t host;
      xbt_dict_t result;
    } host_get_properties;

    struct {
      smx_host_t host;
      double result;
    } host_get_speed;

    struct {
      smx_host_t host;
      double result;
    } host_get_available_speed;

    struct {
      smx_host_t host;
      int result;
    } host_get_state;

    struct {
      smx_host_t host;
      void* result;
    } host_get_data;

    struct {
      smx_host_t host;
      void* data;
    } host_set_data;

    struct {
      const char* name;
      smx_host_t host;
      double computation_amount;
      double priority;
      smx_action_t result;
    } host_execute;

    struct {
      const char *name;
      int host_nb;
      smx_host_t *host_list;
      double *computation_amount;
      double *communication_amount;
      double amount;
      double rate;
      smx_action_t result;
    } host_parallel_execute;

    struct {
      smx_action_t execution;
    } host_execution_destroy;

    struct {
      smx_action_t execution;
    } host_execution_cancel;

    struct {
      smx_action_t execution;
      double result;
    } host_execution_get_remains;

    struct {
      smx_action_t execution;
      e_smx_state_t result;
    } host_execution_get_state;

    struct {
      smx_action_t execution;
      double priority;
    } host_execution_set_priority;

    struct {
      smx_action_t execution;
      struct s_smx_simcall *simcall;
      e_smx_state_t result;

    } host_execution_wait;

    struct {
      smx_process_t *process;
      const char *name;
      xbt_main_func_t code;
      void *data;
      const char *hostname;
      double kill_time;
      int argc;
      char **argv;
      xbt_dict_t properties;
      int auto_restart;
    } process_create;

    struct {
      smx_process_t process;
    } process_kill;

    struct {
      smx_process_t process;
    } process_cleanup;

    struct {
      smx_process_t process;
      smx_host_t dest;
    } process_change_host;

    struct {
      smx_process_t process;
    } process_suspend;

    struct {
      smx_process_t process;
    } process_resume;

    struct {
      int result;
    } process_count;

    struct {
      smx_process_t process;
      void* result;
    } process_get_data;

    struct {
      smx_process_t process;
      void* data;
    } process_set_data;

    struct {
      smx_process_t process;
      smx_host_t result;
    } process_get_host;

    struct {
      smx_process_t process;
      const char *result;
    } process_get_name;

    struct {
      smx_process_t process;
      int result;
    } process_is_suspended;

    struct {
      smx_process_t process;
      xbt_dict_t result;
    } process_get_properties;

    struct {
      double duration;
      e_smx_state_t result;
    } process_sleep;

    struct {
      smx_process_t process;
      int_f_pvoid_t fun;
      void *data;
    } process_on_exit;

    struct {
      smx_process_t process;
      int auto_restart;
    } process_auto_restart;

    struct {
      smx_process_t process;
      smx_process_t result;
    } process_restart;

    struct {
      const char *name;
      smx_rdv_t result;
    } rdv_create;

    struct {
      smx_rdv_t rdv;
    } rdv_destroy;

    struct {
      const char* name;
      smx_rdv_t result;
    } rdv_get_by_name;

    struct {
      smx_rdv_t rdv;
      smx_host_t host;
      unsigned int result; 
    } rdv_comm_count_by_host;

    struct {
      smx_rdv_t rdv;
      smx_action_t result;
    } rdv_get_head;

    struct {
      smx_rdv_t rdv;
      smx_process_t receiver;
    } rdv_set_rcv_proc;

    struct {
      smx_rdv_t rdv;
      smx_process_t result;
    } rdv_get_rcv_proc;

    struct {
      smx_rdv_t rdv;
      double task_size;
      double rate;
      void *src_buff;
      size_t src_buff_size;
      int (*match_fun)(void *, void *, smx_action_t);
      void *data;
      double timeout;
    } comm_send;

    struct {
      smx_rdv_t rdv;
      double task_size;
      double rate;
      void *src_buff;
      size_t src_buff_size;
      int (*match_fun)(void *, void *, smx_action_t);
      void (*clean_fun)(void *);
      void *data;
      int detached;
      smx_action_t result;
    } comm_isend;

    struct {
      smx_rdv_t rdv;
      void *dst_buff;
      size_t *dst_buff_size;
      int (*match_fun)(void *, void *, smx_action_t);
      void *data;
      double timeout;
    } comm_recv;

    struct {
      smx_rdv_t rdv;
      void *dst_buff;
      size_t *dst_buff_size;
      int (*match_fun)(void *, void *, smx_action_t);
      void *data;
      smx_action_t result;
    } comm_irecv;

    struct {
      smx_rdv_t rdv;
      int src;
      int tag;
      int (*match_fun)(void *, void *, smx_action_t);
      void *data;
      smx_action_t result;
    } comm_iprobe;

    struct {
      smx_action_t comm;
    } comm_destroy;

    struct {
      smx_action_t comm;
    } comm_cancel;

    struct {
      xbt_dynar_t comms;
      unsigned int result;
    } comm_waitany;

    struct {
      smx_action_t comm;
      double timeout;
    } comm_wait;

    struct {
      smx_action_t comm;
      int result;
    } comm_test;

    struct {
      xbt_dynar_t comms;
      int result;
    } comm_testany;

    struct {
      smx_action_t comm;
      double result;
    } comm_get_remains;

    struct {
      smx_action_t comm;
      e_smx_state_t result;
    } comm_get_state;

    struct {
      smx_action_t comm;
      void *result;
    } comm_get_src_data;

    struct {
      smx_action_t comm;
      void *result;
    } comm_get_dst_data;

    struct {
      smx_action_t comm;
      smx_process_t result;
    } comm_get_src_proc;

    struct {
      smx_action_t comm;
      smx_process_t result;
    } comm_get_dst_proc;

#ifdef HAVE_LATENCY_BOUND_TRACKING
    struct {
      smx_action_t comm;
      int result;
    } comm_is_latency_bounded;
#endif

#ifdef HAVE_TRACING
    struct {
      smx_action_t action;
      const char *category;
    } set_category;
#endif

    struct {
      smx_mutex_t result;
    } mutex_init;

    struct {
      smx_mutex_t mutex;
    } mutex_lock;

    struct {
      smx_mutex_t mutex;
      int result;
    } mutex_trylock;

    struct {
      smx_mutex_t mutex;
    } mutex_unlock;

    struct {
      smx_mutex_t mutex;
    } mutex_destroy;

    struct {
      smx_cond_t result;
    } cond_init;

    struct {
      smx_cond_t cond;
    } cond_destroy;

    struct {
      smx_cond_t cond;
    } cond_signal;

    struct {
      smx_cond_t cond;
      smx_mutex_t mutex;
    } cond_wait;

    struct {
      smx_cond_t cond;
      smx_mutex_t mutex;
      double timeout;
    } cond_wait_timeout;

    struct {
      smx_cond_t cond;
    } cond_broadcast;

    struct {
      int capacity;
      smx_sem_t result;
    } sem_init;

    struct {
      smx_sem_t sem;
    } sem_destroy;

    struct {
      smx_sem_t sem;
    } sem_release;

    struct {
      smx_sem_t sem;
      int result;
    } sem_would_block;

    struct {
      smx_sem_t sem;
    } sem_acquire;

    struct {
      smx_sem_t sem;
      double timeout;
    } sem_acquire_timeout;

    struct {
      smx_sem_t sem;
      int result;
    } sem_get_capacity;

    struct {
      void *ptr;
      size_t size;
      size_t nmemb;
      smx_file_t stream;
      double result;
    } file_read;

    struct {
      const void *ptr;
      size_t size;
      size_t nmemb;
      smx_file_t stream;
      size_t result;
    } file_write;

    struct {
      const char* mount;
      const char* path;
      const char* mode;
      smx_file_t result;
    } file_open;

    struct {
      smx_file_t fp;
      int result;
    } file_close;

    struct {
      smx_file_t fd;
      s_file_stat_t buf;
      int result;
    } file_stat;

    struct {
      smx_file_t fd;
      int result;
    } file_unlink;

    struct {
      const char *mount;
      const char *path;
      xbt_dict_t result;
    } file_ls;

    struct {
      const char* name;
      xbt_dict_t result;
    } asr_get_properties;

    struct{
      void *s;
    } mc_snapshot;

    struct{
      void *snapshot1;
      void *snapshot2;
      int result;
    } mc_compare_snapshots;

    /* ****************************************************************************************** */
    /* TUTORIAL: New API                                                                        */
    /* ****************************************************************************************** */
    struct {
      const char* param1;
      double param2;
      int result;
    } new_api;

  };
} s_smx_simcall_t, *smx_simcall_t;



/******************************** General *************************************/

void SIMIX_simcall_push(smx_process_t self);
void SIMIX_simcall_answer(smx_simcall_t);
void SIMIX_simcall_pre(smx_simcall_t, int);
void SIMIX_simcall_post(smx_action_t);
smx_simcall_t SIMIX_simcall_mine(void);
const char *SIMIX_simcall_name(e_smx_simcall_t kind);
//TOFIX put it in a better place
xbt_dict_t SIMIX_pre_asr_get_properties(smx_simcall_t simcall, const char *name);

/*************************** New simcall interface ****************************/

#define SIMIX_pack_args(...) (u_smx_scalar_t[]){MAP(INIT_FIELD, __VA_ARGS__)}


/*
 * Define scalar type wrappers to ease the use of simcalls.
 * These are used to wrap the arguments in SIMIX_simcall macro.
 */
#define CHAR(x) (c,x)
#define STRING(x) (cc,x)
#define SHORT(x) (s,x)
#define INT(x) (i,x)
#define LONG(x) (l,x)
#define UCHAR(x) (uc,x)
#define USHORT(x) (us,x)
#define UINT(x) (ui,x)
#define ULONG(x) (ul,x)
#define FLOAT(x) (f,x)
#define DOUBLE(x) (d,x)
#define PTR(x)  (p,x)

/*
 * Some macro machinery to get a MAP over the arguments of a variadic macro.
 * It uses a FOLD to apply a macro to every argument, and because there is
 * no recursion in the C preprocessor we must create a new macro for every
 * depth of FOLD's recursion.
 */

/* FOLD macro */
#define FE_0(WHAT, X, ...)
#define FE_1(I, WHAT, X) WHAT(I, X)
#define FE_2(I, WHAT, X, ...) WHAT(I, X), FE_1(I+1, WHAT, __VA_ARGS__)
#define FE_3(I, WHAT, X, ...) WHAT(I, X), FE_2(I+1, WHAT, __VA_ARGS__)
#define FE_4(I, WHAT, X, ...) WHAT(I, X), FE_3(I+1, WHAT, __VA_ARGS__)
#define FE_5(I, WHAT, X, ...) WHAT(I, X), FE_4(I+1, WHAT, __VA_ARGS__)
#define FE_6(I, WHAT, X, ...) WHAT(I, X), FE_5(I+1, WHAT, __VA_ARGS__)
#define FE_7(I, WHAT, X, ...) WHAT(I, X), FE_6(I+1, WHAT, __VA_ARGS__)
#define FE_8(I, WHAT, X, ...) WHAT(I, X), FE_7(I+1, WHAT, __VA_ARGS__)
#define FE_9(I, WHAT, X, ...) WHAT(I, X), FE_8(I+1, WHAT, __VA_ARGS__)
#define FE_10(I, WHAT, X, ...) WHAT(I, X), FE_9(I+1, WHAT, __VA_ARGS__)


/* NOTE: add as many FE_n as needed (maximum number of simcall arguments )*/

/* Make a MAP macro usgin FOLD (will apply 'action' to the arguments.
 * GET_MACRO is a smart hack that counts the number of arguments passed to
 * the variadic macro, and it is used to invoke the right FOLD depth.
 */
#define GET_MACRO(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,NAME,...) NAME
#define MAP(action, ...) \
  GET_MACRO(, ##__VA_ARGS__, FE_10,FE_9,FE_8,FE_7,FE_6,FE_5,FE_4,FE_3,FE_2,FE_1, FE_0) (0, action, __VA_ARGS__)

/* Generate code to initialize the field 'x' with value 'y' of an structure or union */
#define INIT_FIELD_(x,y) {.x = y}
#define INIT_FIELD(t) INIT_FIELD_ t

/* Project the second element of a tuple */
#define SECOND_(x, y) y
#define SECOND(t) SECOND_ t

/*
 * \brief Simcall invocation macro
 * It calls a dummy function that uses the format attribute to ensure typesafety (see
 * gcc format attribute), then it invokes the real simcall function packing the
 * user provided arguments in an array.
 * \param id a simcall id (from the simcall enumeration ids)
 *
 */
#define SIMIX_simcall(id, ...) \
  SIMIX_simcall_typecheck(simcall_types[id], MAP(SECOND, __VA_ARGS__)); \
  __SIMIX_simcall(id, (u_smx_scalar_t[]){MAP(INIT_FIELD, __VA_ARGS__)})

smx_simcall_t __SIMIX_simcall(e_smx_simcall_t simcall_id, u_smx_scalar_t *args);

/*
 * \biref Dummy variadic function used to typecheck the arguments of a simcall
 * \param fmt A format string following printf style
 */
void SIMIX_simcall_typecheck(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

typedef smx_action_t (*simcall_handler_t)(u_smx_scalar_t *);

extern const char *simcall_types[];
extern simcall_handler_t simcall_table[];

#endif

