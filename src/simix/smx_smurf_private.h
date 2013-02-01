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

/* MAP with default arguments */
#define APPLY_MAP(WHAT, I, X, ...) WHAT(I, __VA_ARGS__, X)
#define FE_DA_0(I, WHAT, args, X, ...)
#define FE_DA_1(I, WHAT, args, X, ...) APPLY_MAP(WHAT, I, X, args)
#define FE_DA_2(I, WHAT, args, X, ...) APPLY_MAP(WHAT, I, X, args) FE_DA_1(I+1, WHAT, args, __VA_ARGS__)
#define FE_DA_3(I, WHAT, args, X, ...) APPLY_MAP(WHAT, I, X, args) FE_DA_2(I+1, WHAT, args, __VA_ARGS__)
#define FE_DA_4(I, WHAT, args, X, ...) APPLY_MAP(WHAT, I, X, args) FE_DA_3(I+1, WHAT, args, __VA_ARGS__)
#define FE_DA_5(I, WHAT, args, X, ...) APPLY_MAP(WHAT, I, X, args) FE_DA_4(I+1, WHAT, args, __VA_ARGS__)
#define FE_DA_6(I, WHAT, args, X, ...) APPLY_MAP(WHAT, I, X, args) FE_DA_5(I+1, WHAT, args, __VA_ARGS__)
#define FE_DA_7(I, WHAT, args, X, ...) APPLY_MAP(WHAT, I, X, args) FE_DA_6(I+1, WHAT, args, __VA_ARGS__)
#define FE_DA_8(I, WHAT, args, X, ...) APPLY_MAP(WHAT, I, X, args) FE_DA_7(I+1, WHAT, args, __VA_ARGS__)
#define FE_DA_9(I, WHAT, args, X, ...) APPLY_MAP(WHAT, I, X, args) FE_DA_8(I+1, WHAT, args, __VA_ARGS__)
#define FE_DA_10(I, WHAT, args, X, ...) APPLY_MAP(WHAT, I, X, args) FE_DA_9(I+1, WHAT, args, __VA_ARGS__)

#define MAP_WITH_DEFAULT_ARGS(action, args, ...) \
  GET_MACRO(, ##__VA_ARGS__, FE_DA_10,FE_DA_9,FE_DA_8,FE_DA_7,FE_DA_6,FE_DA_5,FE_DA_4,FE_DA_3,FE_DA_2,FE_DA_1, FE_DA_0) (0, action, args, __VA_ARGS__)

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

/* get the field of the parameter */
#define SIMCALL_FIELD_(name, type, field) field
#define SIMCALL_FIELD(i, v) SIMCALL_FIELD_ v

/* get the parameter declaration */
#define SIMCALL_ARG_(name, type, field) type name
#define SIMCALL_ARG(i, v) SIMCALL_ARG_ v

/* get the parameter initialisation field */
#define SIMCALL_INIT_FIELD_(name, type, field) .field = name
#define SIMCALL_INIT_FIELD(i, d, v) self->simcall.args[i]SIMCALL_INIT_FIELD_ v;

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

#define SIMCALL_WITH_FUNC_RETURN(name, type, field) return self->simcall.result.field;
#define SIMCALL_WITHOUT_FUNC_RETURN(name, type, field)
#define SIMCALL_FUNC_RETURN_(name, type, ...)\
        MAYBE2(,##__VA_ARGS__, SIMCALL_WITH_FUNC_RETURN, SIMCALL_WITHOUT_FUNC_RETURN)\
	(name, type, __VA_ARGS__)
#define SIMCALL_FUNC_RETURN(res) SIMCALL_FUNC_RETURN_ res


/* generate the simcall enumeration */
#define SIMCALL_ENUM(type, ...)\
	type

/* generate the strings name from the enumeration values */
#define SIMCALL_STRING_TYPE(type, name, answer, res, ...)\
	[type] = STRINGIFY(type)

/* generate strings from the enumeration values */
#define SIMCALL_TYPE(type, name, answer, res, ...)\
	[type] = STRINGIFY(MAP(SIMCALL_FORMAT, __VA_ARGS__))

/* generate the simcalls BODY functions */
#define SIMCALL_FUNC(TYPE, NAME, ANSWER, RES, ...)\
  inline static SIMCALL_FUNC_RETURN_TYPE(RES) simcall_BODY_##NAME(MAP(SIMCALL_ARG, ##__VA_ARGS__)) { \
    smx_process_t self = SIMIX_process_self(); \
    self->simcall.call = TYPE; \
    memset(self->simcall.args, 0, sizeof(self->simcall.args));  \
    MAP_WITH_DEFAULT_ARGS(SIMCALL_INIT_FIELD, (), ##__VA_ARGS__) \
    if (self != simix_global->maestro_process) { \
      XBT_DEBUG("Yield process '%s' on simcall %s (%d)", self->name, \
                SIMIX_simcall_name(self->simcall.call), (int)self->simcall.call); \
      SIMIX_process_yield(self); \
    } else { \
      SIMIX_simcall_pre(&self->simcall, 0); \
    } \
    SIMCALL_FUNC_RETURN(RES) \
  }

/* generate a comma if there is an argument*/
#define WITHOUT_COMMA 
#define WITH_COMMA ,
#define GET_CLEAN(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10, NAME,...) NAME
#define MAYBE_COMMA(...) GET_CLEAN(,##__VA_ARGS__,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITH_COMMA,WITHOUT_COMMA)

/* generate the simcalls cases for the SIMIX_simcall_pre function */
#define WITH_ANSWER(...) __VA_ARGS__
#define WITHOUT_ANSWER(...) 
#define SIMCALL_CASE(type, name, answer, res, ...)\
    case type:\
      SIMCALL_RESULT_BEGIN(answer, res) SIMIX_pre_ ## name(simcall MAYBE_COMMA(__VA_ARGS__) MAP(SIMCALL_CASE_PARAM, ##__VA_ARGS__));\
      SIMCALL_RESULT_END(answer, res)\
      break;


/*
 * Generate simcall args and result getter/setter
 */
#define SIMCALL_GS_SC_NAME_(n) n
#define SIMCALL_GS_SC_NAME(n) SIMCALL_GS_SC_NAME_ n
#define SIMCALL_GS_ARG_NAME(n) SIMCALL_NAME_ n
#define JOIN2(_0, _1) _0 ##__## _1
#define JOIN3(_0, _1, _2) JOIN2(_0 ##__## _1, _2)
#define JOIN4(_0, _1, _2, _3) JOIN3(_0 ##_## _1, _2, _3)
#define SIMCALL_GS_FUNC(scname, setget, vname) \
   JOIN4(simcall, scname, setget, vname)

/* generate the simcalls args getter/setter */
#define SIMCALL_ARG_GETSET_(i, name, v) \
  static inline SIMCALL_FUNC_RETURN_TYPE(v) SIMCALL_GS_FUNC(SIMCALL_GS_SC_NAME(name), get, SIMCALL_GS_ARG_NAME(v))(smx_simcall_t simcall){\
    return simcall->args[i].SIMCALL_FIELD_ v ;\
  }\
  static inline void SIMCALL_GS_FUNC(SIMCALL_GS_SC_NAME(name), set, SIMCALL_GS_ARG_NAME(v))(smx_simcall_t simcall, SIMCALL_ARG_ v){\
    simcall->args[i].SIMCALL_FIELD_ v = SIMCALL_NAME_ v ;\
  }

#define SIMCALL_ARG_GETSET(type, name, answer, res, ...)\
    MAP_WITH_DEFAULT_ARGS(SIMCALL_ARG_GETSET_, (name), ##__VA_ARGS__)

/* generate the simcalls result getter/setter */
#define SIMCALL_WITH_RES_GETSET(name, v) \
  static inline SIMCALL_FUNC_RETURN_TYPE(v) SIMCALL_GS_FUNC(SIMCALL_GS_SC_NAME((name)), get, SIMCALL_GS_ARG_NAME(v))(smx_simcall_t simcall){\
    return simcall->result.SIMCALL_FIELD_ v ;\
  }\
  static inline void SIMCALL_GS_FUNC(SIMCALL_GS_SC_NAME((name)), set, SIMCALL_GS_ARG_NAME(v))(smx_simcall_t simcall, SIMCALL_ARG_ v){\
    simcall->result.SIMCALL_FIELD_ v = SIMCALL_NAME_ v ;\
  }
#define SIMCALL_WITHOUT_RES_GETSET(name, v)
#define SIMCALL_RES_GETSET__(name, type, ...)\
        MAYBE2(,##__VA_ARGS__, SIMCALL_WITH_RES_GETSET, SIMCALL_WITHOUT_RES_GETSET)
#define SIMCALL_RES_GETSET_(scname, v)\
        SIMCALL_RES_GETSET__ v (scname, v)
#define SIMCALL_RES_GETSET(type, name, answer, res, ...)\
  SIMCALL_RES_GETSET_(name, res)

/* generate the simcalls result getter/setter protos*/
#define SIMCALL_WITH_RES_GETSET_PROTO(name, v) \
  inline SIMCALL_FUNC_RETURN_TYPE(v) SIMCALL_GS_FUNC(SIMCALL_GS_SC_NAME((name)), get, SIMCALL_GS_ARG_NAME(v))(smx_simcall_t simcall);\
  inline void SIMCALL_GS_FUNC(SIMCALL_GS_SC_NAME((name)), set, SIMCALL_GS_ARG_NAME(v))(smx_simcall_t simcall, SIMCALL_ARG_ v);
#define SIMCALL_WITHOUT_RES_GETSET_PROTO(name, v)
#define SIMCALL_RES_GETSET_PROTO__(name, type, ...)\
        MAYBE2(,##__VA_ARGS__, SIMCALL_WITH_RES_GETSET_PROTO, SIMCALL_WITHOUT_RES_GETSET_PROTO)
#define SIMCALL_RES_GETSET_PROTO_(scname, v)\
        SIMCALL_RES_GETSET_PROTO__ v (scname, v)
#define SIMCALL_RES_GETSET_PROTO(type, name, answer, res, ...)\
  SIMCALL_RES_GETSET_PROTO_(name, res)

/* stringify arguments */
#define STRINGIFY_(...) #__VA_ARGS__
#define STRINGIFY(...) STRINGIFY_(__VA_ARGS__)

/* the list of simcalls definitions */
#define SIMCALL_LIST1(ACTION, sep) \
ACTION(SIMCALL_HOST_GET_BY_NAME, host_get_by_name, WITH_ANSWER, TSPEC(result, smx_host_t), TSTRING(name)) sep \
ACTION(SIMCALL_HOST_GET_NAME, host_get_name, WITH_ANSWER, TSTRING(result), TSPEC(host, smx_host_t)) sep \
ACTION(SIMCALL_HOST_GET_PROPERTIES, host_get_properties, WITH_ANSWER, TSPEC(result, xbt_dict_t), TSPEC(host, smx_host_t)) sep \
ACTION(SIMCALL_HOST_GET_SPEED, host_get_speed, WITH_ANSWER, TDOUBLE(result), TSPEC(host, smx_host_t)) sep \
ACTION(SIMCALL_HOST_GET_AVAILABLE_SPEED, host_get_available_speed, WITH_ANSWER, TDOUBLE(result), TSPEC(host, smx_host_t)) sep \
ACTION(SIMCALL_HOST_GET_STATE, host_get_state, WITH_ANSWER, TINT(result), TSPEC(host, smx_host_t)) sep \
ACTION(SIMCALL_HOST_GET_DATA, host_get_data, WITH_ANSWER, TPTR(result), TSPEC(host, smx_host_t)) sep \
ACTION(SIMCALL_HOST_SET_DATA, host_set_data, WITH_ANSWER, TVOID(result), TSPEC(host, smx_host_t), TPTR(data)) sep \
ACTION(SIMCALL_HOST_EXECUTE, host_execute, WITH_ANSWER, TSPEC(result, smx_action_t), TSTRING(name), TSPEC(host, smx_host_t), TDOUBLE(computation_amount), TDOUBLE(priority)) sep \
ACTION(SIMCALL_HOST_PARALLEL_EXECUTE, host_parallel_execute, WITH_ANSWER, TSPEC(result, smx_action_t), TSTRING(name), TINT(host_nb), TSPEC(host_list, smx_host_t*), TSPEC(computation_amount, double*), TSPEC(communication_amount, double*), TDOUBLE(amount), TDOUBLE(rate)) sep \
ACTION(SIMCALL_HOST_EXECUTION_DESTROY, host_execution_destroy, WITH_ANSWER, TVOID(result), TSPEC(execution, smx_action_t)) sep \
ACTION(SIMCALL_HOST_EXECUTION_CANCEL, host_execution_cancel, WITH_ANSWER, TVOID(result), TSPEC(execution, smx_action_t)) sep \
ACTION(SIMCALL_HOST_EXECUTION_GET_REMAINS, host_execution_get_remains, WITH_ANSWER, TDOUBLE(result), TSPEC(execution, smx_action_t)) sep \
ACTION(SIMCALL_HOST_EXECUTION_GET_STATE, host_execution_get_state, WITH_ANSWER, TINT(result), TSPEC(execution, smx_action_t)) sep \
ACTION(SIMCALL_HOST_EXECUTION_SET_PRIORITY, host_execution_set_priority, WITH_ANSWER, TVOID(result), TSPEC(execution, smx_action_t), TDOUBLE(priority)) sep \
ACTION(SIMCALL_HOST_EXECUTION_WAIT, host_execution_wait, WITHOUT_ANSWER, TINT(result), TSPEC(execution, smx_action_t)) sep \
ACTION(SIMCALL_VM_WS_CREATE, vm_ws_create, WITH_ANSWER, TPTR(result), TSTRING(name), TSPEC(phys_host, smx_host_t)) sep \
ACTION(SIMCALL_VM_START, vm_start, WITHOUT_ANSWER, TVOID(result), TSPEC(ind_phys_host, smx_host_t)) sep \
ACTION(SIMCALL_VM_SET_STATE, vm_set_state, WITHOUT_ANSWER, TVOID(result), TSPEC(ind_vm, smx_host_t), TINT(state)) sep \
ACTION(SIMCALL_VM_GET_STATE, vm_get_state, WITH_ANSWER, TINT(result), TSPEC(ind_vm, smx_host_t)) sep \
ACTION(SIMCALL_VM_DESTROY, vm_destroy, WITHOUT_ANSWER, TVOID(result), TSPEC(ind_vm, smx_host_t)) sep \
ACTION(SIMCALL_VM_SUSPEND, vm_suspend, WITHOUT_ANSWER, TVOID(result), TSPEC(vm, smx_host_t)) sep \
ACTION(SIMCALL_VM_RESUME, vm_resume, WITHOUT_ANSWER, TVOID(result), TSPEC(vm, smx_host_t)) sep \
ACTION(SIMCALL_VM_SHUTDOWN, vm_shutdown, WITHOUT_ANSWER, TVOID(result), TSPEC(vm, smx_host_t)) sep \
ACTION(SIMCALL_PROCESS_CREATE, process_create, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t*), TSTRING(name), TSPEC(code, xbt_main_func_t), TPTR(data), TSTRING(hostname), TDOUBLE(kill_time), TINT(argc), TSPEC(argv, char**), TSPEC(properties, xbt_dict_t), TINT(auto_restart)) sep \
ACTION(SIMCALL_PROCESS_KILL, process_kill, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t)) sep \
ACTION(SIMCALL_PROCESS_KILLALL, process_killall, WITH_ANSWER, TVOID(result), TINT(reset_pid)) sep \
ACTION(SIMCALL_PROCESS_CLEANUP, process_cleanup, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t)) sep \
ACTION(SIMCALL_PROCESS_CHANGE_HOST, process_change_host, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t), TSPEC(dest, smx_host_t)) sep \
ACTION(SIMCALL_PROCESS_SUSPEND, process_suspend, WITHOUT_ANSWER, TVOID(result), TSPEC(process, smx_process_t)) sep \
ACTION(SIMCALL_PROCESS_RESUME, process_resume, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t)) sep \
ACTION(SIMCALL_PROCESS_COUNT, process_count, WITH_ANSWER, TINT(result)) sep \
ACTION(SIMCALL_PROCESS_GET_PID, process_get_PID, WITH_ANSWER, TINT(result), TSPEC(process, smx_process_t)) sep  \
ACTION(SIMCALL_PROCESS_GET_PPID, process_get_PPID, WITH_ANSWER, TINT(result), TSPEC(process, smx_process_t)) sep  \
ACTION(SIMCALL_PROCESS_GET_DATA, process_get_data, WITH_ANSWER, TPTR(result), TSPEC(process, smx_process_t)) sep \
ACTION(SIMCALL_PROCESS_SET_DATA, process_set_data, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t), TPTR(data)) sep \
ACTION(SIMCALL_PROCESS_GET_HOST, process_get_host, WITH_ANSWER, TSPEC(result, smx_host_t), TSPEC(process, smx_process_t)) sep \
ACTION(SIMCALL_PROCESS_GET_NAME, process_get_name, WITH_ANSWER, TSTRING(result), TSPEC(process, smx_process_t)) sep \
ACTION(SIMCALL_PROCESS_IS_SUSPENDED, process_is_suspended, WITH_ANSWER, TINT(result), TSPEC(process, smx_process_t)) sep \
ACTION(SIMCALL_PROCESS_GET_PROPERTIES, process_get_properties, WITH_ANSWER, TSPEC(result, xbt_dict_t), TSPEC(process, smx_process_t)) sep \
ACTION(SIMCALL_PROCESS_SLEEP, process_sleep, WITHOUT_ANSWER, TINT(result), TDOUBLE(duration)) sep \
ACTION(SIMCALL_PROCESS_ON_EXIT, process_on_exit, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t), TSPEC(fun, int_f_pvoid_t), TPTR(data)) sep \
ACTION(SIMCALL_PROCESS_AUTO_RESTART_SET, process_auto_restart_set, WITH_ANSWER, TVOID(result), TSPEC(process, smx_process_t), TINT(auto_restart)) sep \
ACTION(SIMCALL_PROCESS_RESTART, process_restart, WITH_ANSWER, TSPEC(result, smx_process_t), TSPEC(process, smx_process_t)) sep \
ACTION(SIMCALL_RDV_CREATE, rdv_create, WITH_ANSWER, TSPEC(result, smx_rdv_t), TSTRING(name)) sep \
ACTION(SIMCALL_RDV_DESTROY, rdv_destroy, WITH_ANSWER, TVOID(result), TSPEC(rdv, smx_rdv_t)) sep \
ACTION(SIMCALL_RDV_GET_BY_NAME, rdv_get_by_name, WITH_ANSWER, TSPEC(result, smx_host_t), TSTRING(name)) sep \
ACTION(SIMCALL_RDV_COMM_COUNT_BY_HOST, rdv_comm_count_by_host, WITH_ANSWER, TUINT(result), TSPEC(rdv, smx_rdv_t), TSPEC(host, smx_host_t)) sep \
ACTION(SIMCALL_RDV_GET_HEAD, rdv_get_head, WITH_ANSWER, TSPEC(result, smx_action_t), TSPEC(rdv, smx_rdv_t)) sep \
ACTION(SIMCALL_RDV_SET_RECV, rdv_set_receiver, WITH_ANSWER, TVOID(result), TSPEC(rdv, smx_rdv_t), TSPEC(receiver, smx_process_t)) sep \
ACTION(SIMCALL_RDV_GET_RECV, rdv_get_receiver, WITH_ANSWER, TSPEC(result, smx_process_t), TSPEC(rdv, smx_rdv_t)) sep \
ACTION(SIMCALL_COMM_IPROBE, comm_iprobe, WITH_ANSWER, TSPEC(result, smx_action_t), TSPEC(rdv, smx_rdv_t), TINT(src), TINT(tag), TSPEC(match_fun, simix_match_func_t), TPTR(data)) sep \
ACTION(SIMCALL_COMM_SEND, comm_send, WITHOUT_ANSWER, TVOID(result), TSPEC(rdv, smx_rdv_t), TDOUBLE(task_size), TDOUBLE(rate), TPTR(src_buff), TSIZE(src_buff_size), TSPEC(match_fun, simix_match_func_t), TPTR(data), TDOUBLE(timeout)) sep \
ACTION(SIMCALL_COMM_ISEND, comm_isend, WITH_ANSWER, TSPEC(result, smx_action_t), TSPEC(rdv, smx_rdv_t), TDOUBLE(task_size), TDOUBLE(rate), TPTR(src_buff), TSIZE(src_buff_size), TSPEC(match_fun, simix_match_func_t), TSPEC(clean_fun, simix_clean_func_t), TPTR(data), TINT(detached)) sep \
ACTION(SIMCALL_COMM_RECV, comm_recv, WITHOUT_ANSWER, TVOID(result), TSPEC(rdv, smx_rdv_t), TPTR(dst_buff), TSPEC(dst_buff_size, size_t*), TSPEC(match_fun, simix_match_func_t), TPTR(data), TDOUBLE(timeout)) sep \
ACTION(SIMCALL_COMM_IRECV, comm_irecv, WITH_ANSWER, TSPEC(result, smx_action_t), TSPEC(rdv, smx_rdv_t), TPTR(dst_buff), TSPEC(dst_buff_size, size_t*), TSPEC(match_fun, simix_match_func_t), TPTR(data)) sep \
ACTION(SIMCALL_COMM_DESTROY, comm_destroy, WITH_ANSWER, TVOID(result), TSPEC(comm, smx_action_t)) sep \
ACTION(SIMCALL_COMM_CANCEL, comm_cancel, WITH_ANSWER, TVOID(result), TSPEC(comm, smx_action_t)) sep \
ACTION(SIMCALL_COMM_WAITANY, comm_waitany, WITHOUT_ANSWER, TINT(result), TSPEC(comms, xbt_dynar_t)) sep \
ACTION(SIMCALL_COMM_WAIT, comm_wait, WITHOUT_ANSWER, TVOID(result), TSPEC(comm, smx_action_t), TDOUBLE(timeout)) sep \
ACTION(SIMCALL_COMM_TEST, comm_test, WITHOUT_ANSWER, TINT(result), TSPEC(comm, smx_action_t)) sep \
ACTION(SIMCALL_COMM_TESTANY, comm_testany, WITHOUT_ANSWER, TINT(result), TSPEC(comms, xbt_dynar_t)) sep \
ACTION(SIMCALL_COMM_GET_REMAINS, comm_get_remains, WITH_ANSWER, TDOUBLE(result), TSPEC(comm, smx_action_t)) sep \
ACTION(SIMCALL_COMM_GET_STATE, comm_get_state, WITH_ANSWER, TINT(result), TSPEC(comm, smx_action_t)) sep \
ACTION(SIMCALL_COMM_GET_SRC_DATA, comm_get_src_data, WITH_ANSWER, TPTR(result), TSPEC(comm, smx_action_t)) sep \
ACTION(SIMCALL_COMM_GET_DST_DATA, comm_get_dst_data, WITH_ANSWER, TPTR(result), TSPEC(comm, smx_action_t)) sep \
ACTION(SIMCALL_COMM_GET_SRC_PROC, comm_get_src_proc, WITH_ANSWER, TSPEC(result, smx_process_t), TSPEC(comm, smx_action_t)) sep \
ACTION(SIMCALL_COMM_GET_DST_PROC, comm_get_dst_proc, WITH_ANSWER, TSPEC(result, smx_process_t), TSPEC(comm, smx_action_t)) sep \
ACTION(SIMCALL_MUTEX_INIT, mutex_init, WITH_ANSWER, TSPEC(result, smx_mutex_t)) sep \
ACTION(SIMCALL_MUTEX_DESTROY, mutex_destroy, WITH_ANSWER, TVOID(result), TSPEC(mutex, smx_mutex_t)) sep \
ACTION(SIMCALL_MUTEX_LOCK, mutex_lock, WITHOUT_ANSWER, TVOID(result), TSPEC(mutex, smx_mutex_t)) sep \
ACTION(SIMCALL_MUTEX_TRYLOCK, mutex_trylock, WITH_ANSWER, TINT(result), TSPEC(mutex, smx_mutex_t)) sep \
ACTION(SIMCALL_MUTEX_UNLOCK, mutex_unlock, WITH_ANSWER, TVOID(result), TSPEC(mutex, smx_mutex_t)) sep \
ACTION(SIMCALL_COND_INIT, cond_init, WITH_ANSWER, TSPEC(result, smx_cond_t)) sep \
ACTION(SIMCALL_COND_DESTROY, cond_destroy, WITH_ANSWER, TVOID(result), TSPEC(cond, smx_cond_t)) sep \
ACTION(SIMCALL_COND_SIGNAL, cond_signal, WITH_ANSWER, TVOID(result), TSPEC(cond, smx_cond_t)) sep \
ACTION(SIMCALL_COND_WAIT, cond_wait, WITHOUT_ANSWER, TVOID(result), TSPEC(cond, smx_cond_t), TSPEC(mutex, smx_mutex_t)) sep \
ACTION(SIMCALL_COND_WAIT_TIMEOUT, cond_wait_timeout, WITHOUT_ANSWER, TVOID(result), TSPEC(cond, smx_cond_t), TSPEC(mutex, smx_mutex_t), TDOUBLE(timeout)) sep \
ACTION(SIMCALL_COND_BROADCAST, cond_broadcast, WITH_ANSWER, TVOID(result), TSPEC(cond, smx_cond_t)) sep \
ACTION(SIMCALL_SEM_INIT, sem_init, WITH_ANSWER, TSPEC(result, smx_sem_t), TINT(capacity)) sep \
ACTION(SIMCALL_SEM_DESTROY, sem_destroy, WITH_ANSWER, TVOID(result), TSPEC(sem, smx_sem_t)) sep \
ACTION(SIMCALL_SEM_RELEASE, sem_release, WITH_ANSWER, TVOID(result), TSPEC(sem, smx_sem_t)) sep \
ACTION(SIMCALL_SEM_WOULD_BLOCK, sem_would_block, WITH_ANSWER, TINT(result), TSPEC(sem, smx_sem_t)) sep \
ACTION(SIMCALL_SEM_ACQUIRE, sem_acquire, WITHOUT_ANSWER, TVOID(result), TSPEC(sem, smx_sem_t)) sep \
ACTION(SIMCALL_SEM_ACQUIRE_TIMEOUT, sem_acquire_timeout, WITHOUT_ANSWER, TVOID(result), TSPEC(sem, smx_sem_t), TDOUBLE(timeout)) sep \
ACTION(SIMCALL_SEM_GET_CAPACITY, sem_get_capacity, WITH_ANSWER, TINT(result), TSPEC(sem, smx_sem_t)) sep \
ACTION(SIMCALL_FILE_READ, file_read, WITHOUT_ANSWER, TDOUBLE(result), TPTR(ptr), TSIZE(size), TSIZE(nmemb), TSPEC(stream, smx_file_t)) sep \
ACTION(SIMCALL_FILE_WRITE, file_write, WITHOUT_ANSWER, TSIZE(result), TCPTR(ptr), TSIZE(size), TSIZE(nmemb), TSPEC(stream, smx_file_t)) sep \
ACTION(SIMCALL_FILE_OPEN, file_open, WITHOUT_ANSWER, TSPEC(result, smx_file_t), TSTRING(mount), TSTRING(path), TSTRING(mode)) sep \
ACTION(SIMCALL_FILE_CLOSE, file_close, WITHOUT_ANSWER, TINT(result), TSPEC(fp, smx_file_t)) sep \
ACTION(SIMCALL_FILE_STAT, file_stat, WITHOUT_ANSWER, TINT(result), TSPEC(fd, smx_file_t), TSPEC(buf, s_file_stat_t*)) sep \
ACTION(SIMCALL_FILE_UNLINK, file_unlink, WITHOUT_ANSWER, TINT(result), TSPEC(fd, smx_file_t)) sep \
ACTION(SIMCALL_FILE_LS, file_ls, WITHOUT_ANSWER, TSPEC(result, xbt_dict_t), TSTRING(mount), TSTRING(path)) sep \
ACTION(SIMCALL_ASR_GET_PROPERTIES, asr_get_properties, WITH_ANSWER, TSPEC(result, xbt_dict_t), TSTRING(name)) sep 

/* SIMCALL_COMM_IS_LATENCY_BOUNDED and SIMCALL_SET_CATEGORY make things complicated
 * because they are not always present */
#ifdef HAVE_LATENCY_BOUND_TRACKING
#define SIMCALL_LIST2(ACTION, sep) \
ACTION(SIMCALL_COMM_IS_LATENCY_BOUNDED, comm_is_latency_bounded, WITH_ANSWER, TINT(result), TSPEC(comm, smx_action_t)) sep
#else
#define SIMCALL_LIST2(ACTION, sep)
#endif

#ifdef HAVE_TRACING
#define SIMCALL_LIST3(ACTION, sep) \
ACTION(SIMCALL_SET_CATEGORY, set_category, WITH_ANSWER, TVOID(result), TSPEC(action, smx_action_t), TSTRING(category)) sep
#else
#define SIMCALL_LIST3(ACTION, sep)
#endif

#ifdef HAVE_MC
#define SIMCALL_LIST4(ACTION, sep) \
ACTION(SIMCALL_MC_SNAPSHOT, mc_snapshot, WITH_ANSWER, TPTR(result)) sep \
ACTION(SIMCALL_MC_COMPARE_SNAPSHOTS, mc_compare_snapshots, WITH_ANSWER, TINT(result), TPTR(s1), TPTR(s2)) sep 
#else
#define SIMCALL_LIST4(ACTION, sep)
#endif

/* SIMCALL_LIST is the final macro to use */
#define SIMCALL_LIST(ACTION, ...) \
  SIMCALL_LIST1(ACTION, ##__VA_ARGS__)\
  SIMCALL_LIST2(ACTION, ##__VA_ARGS__)\
  SIMCALL_LIST3(ACTION, ##__VA_ARGS__)\
  SIMCALL_LIST4(ACTION, ##__VA_ARGS__)


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
  union u_smx_scalar args[10];
  union u_smx_scalar result;
  //FIXME: union u_smx_scalar retval;
  union {
    struct {
      const char* param1;
      double param2;
      int result;
    } new_api;

  };
} s_smx_simcall_t, *smx_simcall_t;

SIMCALL_LIST(SIMCALL_RES_GETSET, SIMCALL_SEP_NOTHING)
SIMCALL_LIST(SIMCALL_ARG_GETSET, SIMCALL_SEP_NOTHING)

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

smx_simcall_t __SIMIX_simcall(e_smx_simcall_t simcall_id, u_smx_scalar_t *args);

typedef smx_action_t (*simcall_handler_t)(u_smx_scalar_t *);

extern const char *simcall_types[];
extern simcall_handler_t simcall_table[];

#endif

