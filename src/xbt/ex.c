/* $Id$ */

/* ex - Exception Handling (modified to fit into SimGrid from OSSP version) */

/*  Copyright (c) 2005, 2006, 2007 Martin Quinson                           */
/*  Copyright (c) 2002-2004 Ralf S. Engelschall <rse@engelschall.com>       */
/*  Copyright (c) 2002-2004 The OSSP Project <http://www.ossp.org/>         */
/*  Copyright (c) 2002-2004 Cable & Wireless <http://www.cw.com/>           */
/*  All rights reserved.                                                    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>

#include "portable.h" /* execinfo when available */
#include "xbt/ex.h"
#include "xbt/str.h"
#include "xbt/module.h" /* xbt_binary_name */
#include "xbt/synchro.h" /* xbt_thread_self */

#include "gras/Virtu/virtu_interface.h" /* gras_os_myname */
#include "xbt/ex_interface.h"

#if (defined(WIN32) && defined(_M_IX86))
#include <dbghelp.h>
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_ex,xbt,"Exception mecanism");

#if (defined(WIN32) && defined(_M_IX86))

/* 
 * Win32 (x86) implementation backtrace, backtrace_symbols, backtrace_symbols_fd
 * : support for application self-debugging.
 */

/* Pointer function to SymInitialize() */
typedef BOOL (WINAPI *xbt_pfn_sym_initialize_t)(HANDLE, PSTR , BOOL);

/* Pointer function to SymCleanup() */
typedef BOOL (WINAPI *xbt_pfn_sym_cleanup_t)(HANDLE hProcess);

/* Pointer function to SymFunctionTableAccess() */
typedef PVOID (WINAPI *xbt_pfn_sym_function_table_access_t)(HANDLE, DWORD);

/* Pointer function to SymGetLineFromAddr() */
typedef BOOL (WINAPI *xbt_pfn_sym_get_line_from_addr_t)(HANDLE, DWORD, PDWORD , PIMAGEHLP_LINE);

/* Pointer function to SymGetModuleBase() */
typedef DWORD (WINAPI *xbt_pfn_sym_get_module_base_t)(HANDLE,DWORD);

/* Pointer function to SymGetOptions() */
typedef DWORD (WINAPI *xbt_pfn_sym_get_options_t)(VOID);

/* Pointer function to SymGetSymFromAddr() */
typedef BOOL (WINAPI *xbt_pfn_sym_get_sym_from_addr_t)(HANDLE, DWORD, PDWORD , OUT PIMAGEHLP_SYMBOL);

/* Pointer function to SymSetOptions() */
typedef DWORD (WINAPI *xbt_pfn_sym_set_options_t)(DWORD);

/* Pointer function to StackWalk() */
typedef BOOL (WINAPI *xbt_pfn_stack_walk_t)(DWORD, HANDLE, HANDLE ,LPSTACKFRAME, PVOID,PREAD_PROCESS_MEMORY_ROUTINE, PFUNCTION_TABLE_ACCESS_ROUTINE, PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE);

/* This structure representes the debug_help library used interface */
typedef struct s_xbt_debug_help
{
	HINSTANCE instance;
	HANDLE process_handle;											
	xbt_pfn_sym_initialize_t sym_initialize;
	xbt_pfn_sym_cleanup_t sym_cleanup;
	xbt_pfn_sym_function_table_access_t sym_function_table_access;
	xbt_pfn_sym_get_line_from_addr_t sym_get_line_from_addr;
	xbt_pfn_sym_get_module_base_t sym_get_module_base;
	xbt_pfn_sym_get_options_t sym_get_options;
	xbt_pfn_sym_get_sym_from_addr_t sym_get_sym_from_addr;
	xbt_pfn_sym_set_options_t sym_set_options;
	xbt_pfn_stack_walk_t stack_walk;
}s_xbt_debug_hlp_t,* xbt_debug_hlp_t;


/* the address to the unique reference to the debug help library interface */
static xbt_debug_hlp_t 
dbg_hlp = NULL;

/* initialize the debug help library */
static int
dbg_hlp_init(HANDLE process_handle);

/* finalize the debug help library */
static int
dbg_hlp_finalize(void);

/*
 * backtrace() function.
 *
 * Returns a backtrace for the calling program, in  the  array
 * pointed  to  by  buffer.  A backtrace is the series of currently active
 * function calls for the program.  Each item in the array pointed  to  by
 * buffer  is  of  type  void *, and is the return address from the corre-
 * sponding stack frame.  The size argument specifies the  maximum  number
 * of  addresses that can be stored in buffer.  If the backtrace is larger
 * than size, then the addresses corresponding to  the  size  most  recent
 * function  calls  are  returned;  to obtain the complete backtrace, make
 * sure that buffer and size are large enough.
 */

int 
backtrace (void **buffer, int size);

/*
 * backtrace_symbols() function.
 *
 * Given the set of addresses returned by  backtrace()  in  buffer,  back-
 * trace_symbols()  translates the addresses into an array of strings containing
 * the name, the source file and the line number or the las called functions.
 */
char ** 
backtrace_symbols (void *const *buffer, int size);

/*
 * backtrace_symbols_fd() function.
 *
 * Same as backtrace_symbols() function but, the the array of strings is wrotten
 * in a file (fd is the file descriptor of this file)
 */
void 
backtrace_symbols_fd(void *const *buffer, int size, int fd);
#endif

/* default __ex_ctx callback function */
ex_ctx_t *__xbt_ex_ctx_default(void) {
  /* Don't scream: this is a default which is never used (so, yes, 
     there is one setjump container by running entity).

     This default gets overriden in xbt/xbt_os_thread.c so that it works in
     real life and in simulation when using threads to implement the simulation
     processes (ie, with pthreads and on windows).

     It also gets overriden in xbt/context.c when using ucontextes (as well as
     in Java for now, but after the java overhaul, it will get cleaned out)
  */
    static ex_ctx_t ctx = XBT_CTX_INITIALIZER;

    return &ctx;
}

/* Change raw libc symbols to file names and line numbers */
void xbt_ex_setup_backtrace(xbt_ex_t *e);

void xbt_backtrace_current(xbt_ex_t *e) {
#if defined(HAVE_EXECINFO_H) && defined(HAVE_POPEN) && defined(ADDR2LINE) || (defined(WIN32) && defined(_M_IX86))
  e->used     = backtrace((void**)e->bt,XBT_BACKTRACE_SIZE);
  e->bt_strings = NULL;
  xbt_ex_setup_backtrace(e);
#endif
}

void xbt_backtrace_display(xbt_ex_t *e) {
#if defined(HAVE_EXECINFO_H) && defined(HAVE_POPEN) && defined(ADDR2LINE) || (defined(WIN32) && defined(_M_IX86))
  int i;

  if (e->used == 0) {
     fprintf(stderr,"(backtrace not set)\n");
  } else {	
     fprintf(stderr,"Backtrace (displayed in thread %p):\n",
	     (void*)xbt_thread_self());
     for (i=1; i<e->used; i++) /* no need to display "xbt_display_backtrace" */
       fprintf(stderr,"---> %s\n",e->bt_strings[i] +4);
  }
   
  /* don't fool xbt_ex_free with uninitialized msg field */
  e->msg=NULL;
  e->remote=0;
  xbt_ex_free(*e);
#else 

  ERROR0("No backtrace on this arch");
#endif
}

/** \brief show the backtrace of the current point (lovely while debuging) */
void xbt_backtrace_display_current(void) {
  xbt_ex_t e;
  xbt_backtrace_current(&e);
  xbt_backtrace_display(&e);
}

#ifndef WIN32
extern char **environ; /* the environment, as specified by the opengroup */
#endif

void xbt_ex_setup_backtrace(xbt_ex_t *e)  {
#if defined(HAVE_EXECINFO_H) && defined(HAVE_POPEN) && defined(ADDR2LINE)
  int i;
  /* to get the backtrace from the libc */
  char **backtrace = backtrace_symbols (e->bt, e->used);
  
  /* To build the commandline of addr2line */
  char *cmd, *curr;
  
  /* to extract the addresses from the backtrace */
  char **addrs=xbt_new(char*,e->used);
  char buff[256],*p;
  
  /* To read the output of addr2line */
  FILE *pipe;
  char line_func[1024],line_pos[1024];

  /* size (in char) of pointers on this arch */
  int addr_len=0;

  /* To search for the right executable path when not trivial */
  struct stat stat_buf;
  char *binary_name = NULL;
   
  /* Some arches only have stubs of backtrace, no implementation (hppa comes to mind) */
  if (!e->used)
     return;
   
  /* build the commandline */
  if (stat(xbt_binary_name,&stat_buf)) {
    /* Damn. binary not in current dir. We'll have to dig the PATH to find it */
    int i;
    for (i=0; environ[i]; i++) {
      if (!strncmp("PATH=",environ[i], 5)) {	
	xbt_dynar_t path=xbt_str_split(environ[i] + 5, ":");
	unsigned int cpt;
	char *data;
	xbt_dynar_foreach(path, cpt, data) {
	  if (binary_name)
	    free(binary_name);
	  binary_name = bprintf("%s/%s",data,xbt_binary_name);
	  if (!stat(binary_name,&stat_buf)) {
	    /* Found. */
	    DEBUG1("Looked in the PATH for the binary. Found %s",binary_name);
	    xbt_dynar_free(&path);
	    break;
	  } 
	}
	if (stat(binary_name,&stat_buf)) {
	  /* not found */
	  e->used = 1;
	  e->bt_strings = xbt_new(char*,1);
	  e->bt_strings[0] = bprintf("(binary '%s' not found the path)",xbt_binary_name);
	  return;
	}
	xbt_dynar_free(&path);
	break;
      }	
    }
  } else {
    binary_name = xbt_strdup(xbt_binary_name);
  }      
  cmd = curr = xbt_new(char,strlen(ADDR2LINE)+25+strlen(binary_name)+32*e->used);
   
  curr += sprintf(curr,"%s -f -e %s ",ADDR2LINE,binary_name);
  free(binary_name);
   
  for (i=0; i<e->used;i++) {
    /* retrieve this address */
    DEBUG2("Retrieving address number %d from '%s'", i, backtrace[i]);
    snprintf(buff,256,"%s",strchr(backtrace[i],'[')+1);
    p=strchr(buff,']');
    *p='\0';
    if (strcmp(buff,"(nil)"))
       addrs[i]=bprintf("%s", buff);
    else
       addrs[i]=bprintf("0x0");
    DEBUG3("Set up a new address: %d, '%s'(%p)", i, addrs[i], addrs[i]);
     
    /* Add it to the command line args */
    curr+=sprintf(curr,"%s ",addrs[i]);
  } 
  addr_len = strlen(addrs[0]);

  /* parse the output and build a new backtrace */
  e->bt_strings = xbt_new(char*,e->used);
  
  VERB1("Fire a first command: '%s'", cmd);
  pipe = popen(cmd, "r");
  if (!pipe) {
    CRITICAL0("Cannot fork addr2line to display the backtrace");
    abort();
  }

  for (i=0; i<e->used; i++) {
    DEBUG2("Looking for symbol %d, addr = '%s'", i, addrs[i]); 
    fgets(line_func,1024,pipe);
    line_func[strlen(line_func)-1]='\0';
    fgets(line_pos,1024,pipe);
    line_pos[strlen(line_pos)-1]='\0';

    if (strcmp("??",line_func)) {
      DEBUG2("Found static symbol %s() at %s", line_func, line_pos);
      e->bt_strings[i] = bprintf("**   In %s() at %s", line_func,line_pos);
    } else {
      /* Damn. The symbol is in a dynamic library. Let's get wild */
      char *maps_name;
      FILE *maps;
      char maps_buff[512];

      long int addr,offset=0;
      char *p,*p2;

      char *subcmd;
      FILE *subpipe;
      int found=0;

      /* let's look for the offset of this library in our addressing space */
      maps_name=bprintf("/proc/%d/maps",(int)getpid());
      maps=fopen(maps_name,"r");

      sscanf(addrs[i],"%lx",&addr);
      sprintf(maps_buff,"%#lx",addr);
      
      if (strcmp(addrs[i],maps_buff)) {
	CRITICAL2("Cannot parse backtrace address '%s' (addr=%#lx)",
		  addrs[i], addr);
      }
      DEBUG2("addr=%s (as string) =%#lx (as number)",addrs[i],addr);

      while (!found) {
	long int first, last;
	if (fgets(maps_buff,512,maps) == NULL) 
	  break;
	if (i==0) {
	  maps_buff[strlen(maps_buff) -1]='\0';
	  DEBUG1("map line: %s", maps_buff);
	}
	sscanf(maps_buff,"%lx",&first);
	p=strchr(maps_buff,'-')+1;
	sscanf(p,"%lx",&last);
	if (first < addr && addr < last) {
	  offset = first;
	  found=1;
	}
	if (found) {	      
	   DEBUG3("%#lx in [%#lx-%#lx]", addr, first,last);
	   DEBUG0("Symbol found, map lines not further displayed (even if looking for next ones)");
	}
      }
      fclose(maps);
      free(maps_name);

      if (!found) {
	VERB0("Problem while reading the maps file. Following backtrace will be mangled.");
	DEBUG1("No dynamic. Static symbol: %s", backtrace[i]);
	e->bt_strings[i] = bprintf("**   In ?? (%s)", backtrace[i]);
	continue;
      }

      /* Ok, Found the offset of the maps line containing the searched symbol. 
	 We now need to substract this from the address we got from backtrace.
      */
      
      free(addrs[i]);
      addrs[i] = bprintf("0x%0*lx",addr_len-2,addr-offset);
      DEBUG2("offset=%#lx new addr=%s",offset,addrs[i]);

      /* Got it. We have our new address. Let's get the library path and we 
	 are set */ 
      p  = xbt_strdup(backtrace[i]);
      if (p[0]=='[') {
	 /* library path not displayed in the map file either... */
	 free(p);
	 sprintf(line_func,"??");
      } else {
	 p2 = strrchr(p,'(');
	 if (p2) *p2= '\0';
	 p2 = strrchr(p,' ');
	 if(p2) *p2= '\0';
      
	 /* Here we go, fire an addr2line up */
	 subcmd = bprintf("%s -f -e %s %s",ADDR2LINE,p, addrs[i]);
	 free(p);
	 VERB1("Fire a new command: '%s'",subcmd);
	 subpipe = popen(subcmd,"r");
	 if (!subpipe) {
	    CRITICAL0("Cannot fork addr2line to display the backtrace");
	    abort();
	 }
	 fgets(line_func,1024,subpipe);
	 line_func[strlen(line_func)-1]='\0';
	 fgets(line_pos,1024,subpipe);
	 line_pos[strlen(line_pos)-1]='\0';
	 pclose(subpipe);
	 free(subcmd);
      }

      /* check whether the trick worked */
      if (strcmp("??",line_func)) {
	DEBUG2("Found dynamic symbol %s() at %s", line_func, line_pos);
	e->bt_strings[i] = bprintf("**   In %s() at %s", line_func,line_pos);
      } else {
	/* damn, nothing to do here. Let's print the raw address */
	DEBUG1("Dynamic symbol not found. Raw address = %s", backtrace[i]);
	e->bt_strings[i] = bprintf("**   In ?? at %s", backtrace[i]);
      }
      
    }
    free(addrs[i]);
     
    /* Mask the bottom of the stack */    
    if (!strncmp("main",line_func,strlen("main")) ||
	!strncmp("xbt_thread_context_wrapper",line_func,strlen("xbt_thread_context_wrapper"))) {
       int j;
       for (j=i+1; j<e->used; j++)
	 free(addrs[j]);
       e->used = i;

       if (!strncmp("xbt_thread_context_wrapper",line_func,strlen("xbt_thread_context_wrapper"))) {
	  e->used++;
	  e->bt_strings[i] = bprintf("**   (in a separate thread)");
       }       
    }
     
    
  }
  pclose(pipe);
  free(addrs);
  free(backtrace);
  free(cmd);
#elif (defined(WIN32) && defined (_M_IX86))
int i;
char **backtrace = backtrace_symbols (e->bt, e->used);
  
  /* parse the output and build a new backtrace */
  e->bt_strings = xbt_new(char*,e->used);
  

  for(i=0; i<e->used; i++) 
  {
      e->bt_strings[i] = strdup(backtrace[i]);
      free(backtrace[i]);
  }
  
  free(backtrace);
#endif
}    

/** @brief shows an exception content and the associated stack if available */
void xbt_ex_display(xbt_ex_t *e)  {
  char *thrower=NULL;

  if (e->remote)
    thrower = bprintf(" on host %s(%d)",e->host,e->pid);

  fprintf(stderr,
	  "** SimGrid: UNCAUGHT EXCEPTION received on %s(%d): category: %s; value: %d\n"
	  "** %s\n"
	  "** Thrown by %s()%s\n",
	  gras_os_myname(),(*xbt_getpid)(),
	  xbt_ex_catname(e->category), e->value, e->msg,
	  e->procname,thrower?thrower:" in this process");
  CRITICAL1("%s",e->msg);

  if (thrower)
    free(thrower);

  if (!e->remote && !e->bt_strings)
    xbt_ex_setup_backtrace(e);

#if (defined(HAVE_EXECINFO_H) && defined(HAVE_POPEN) && defined(ADDR2LINE)) || (defined(WIN32) && defined(_M_IX86))
  /* We have everything to build neat backtraces */
  {
    int i;
    
    fprintf(stderr,"\n");
    for (i=0; i<e->used; i++)
      fprintf(stderr,"%s\n",e->bt_strings[i]);
    
  }
#else
  fprintf(stderr," at %s:%d:%s (no backtrace available on that arch)\n",  
	  e->file,e->line,e->func);
#endif
  xbt_ex_free(*e);
}


/* default __ex_terminate callback function */
void __xbt_ex_terminate_default(xbt_ex_t *e)  {
  xbt_ex_display(e);

  abort();
}

/* the externally visible API */
XBT_EXPORT_NO_IMPORT(ex_ctx_cb_t)  __xbt_ex_ctx       = &__xbt_ex_ctx_default;
XBT_EXPORT_NO_IMPORT(ex_term_cb_t) __xbt_ex_terminate = &__xbt_ex_terminate_default;


void xbt_ex_free(xbt_ex_t e) {
  int i;

  if (e.msg) free(e.msg);
  if (e.remote) {
    free(e.procname);
    free(e.file);
    free(e.func);
    free(e.host);
  }

  if (e.bt_strings) {	
     for (i=0; i<e.used; i++) 
       free((char*)e.bt_strings[i]);
     free((char **)e.bt_strings);
  }
  /* memset(e,0,sizeof(xbt_ex_t)); */
}

/** \brief returns a short name for the given exception category */
const char * xbt_ex_catname(xbt_errcat_t cat) {
  switch (cat) {
  case unknown_error:   return  "unknown_err";
  case arg_error:       return "invalid_arg";
  case mismatch_error:  return "mismatch";
  case not_found_error: return "not found";
  case system_error:    return "system_err";
  case network_error:   return "network_err";
  case timeout_error:   return "timeout";
  case thread_error:    return "thread_err";
  default:              return "INVALID_ERR";
  }
}

#ifndef HAVE_EXECINFO_H
#  if (defined(WIN32) && defined(_M_IX86))
int 
backtrace (void **buffer, int size)
{
	int pos = 0;
	STACKFRAME* stack_frame;
	int first = 1;

	IMAGEHLP_SYMBOL * pSym;
	unsigned long displacement = 0;
	IMAGEHLP_LINE line_info = {0};
	byte __buffer[(sizeof(SYMBOL_INFO) +MAX_SYM_NAME * sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64)];

	CONTEXT context = {CONTEXT_FULL};
	GetThreadContext(GetCurrentThread(), &context);
	
	/* ebp pointe sur la base de la pile */
	/* esp pointe sur le stack pointer <=> sur le dernier élément déposé dans la pile (l'élément courant) */
	_asm call $+5
	_asm pop eax 
	_asm mov context.Eip, eax 
	_asm mov eax, esp 
	_asm mov context.Esp, eax 
	_asm mov context.Ebp, ebp 

	dbg_hlp_init(GetCurrentProcess());
	
	if((NULL == dbg_hlp) || (size <= 0) || (NULL == buffer))
	{
		errno = EINVAL;
		return 0;
	}

	for(pos = 0; pos < size; pos++)
		buffer[pos] = NULL;
	
	pos = 0;

	pSym = (IMAGEHLP_SYMBOL*)__buffer;

	pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
	pSym->MaxNameLength = MAX_SYM_NAME;


	line_info.SizeOfStruct = sizeof(IMAGEHLP_LINE);
	

    while(pos < size)
    {
		stack_frame = (void*)calloc(1,sizeof(STACKFRAME));
		
		if(!stack_frame)
		{
			errno = ENOMEM;
			break;
		}
		
		stack_frame->AddrPC.Offset = context.Eip;
	    stack_frame->AddrPC.Mode = AddrModeFlat;
	
	    stack_frame->AddrFrame.Offset = context.Ebp;
	    stack_frame->AddrFrame.Mode = AddrModeFlat;
	
	    stack_frame->AddrStack.Offset = context.Esp;
	    stack_frame->AddrStack.Mode = AddrModeFlat;
		
		if((*(dbg_hlp->stack_walk))(
					IMAGE_FILE_MACHINE_I386,
					dbg_hlp->process_handle,
					GetCurrentThread(),
					stack_frame, 
					&context,
					NULL,
					dbg_hlp->sym_function_table_access,
					dbg_hlp->sym_get_module_base,
					NULL)
			&& !first) 
		{
			if(stack_frame->AddrReturn.Offset)
			{

				if((*(dbg_hlp->sym_get_sym_from_addr))(dbg_hlp->process_handle,stack_frame->AddrPC.Offset, &displacement,pSym))
				{	
					if((*(dbg_hlp->sym_get_line_from_addr))(dbg_hlp->process_handle,stack_frame->AddrPC.Offset, &displacement,&line_info))
						buffer[pos++] = (void*)stack_frame;
				}
			}
			else
			{
				free(stack_frame); /* no symbol or no line info */
				break;
			}
		}
		else
		{
			free(stack_frame);

			if(first)
				first = 0;
			else
				break;
		}		
    }

    return pos;
}

char ** 
backtrace_symbols (void *const *buffer, int size)
{
	int pos;
	int success = 0;	
	char** strings;
	STACKFRAME* stack_frame;
	char str[MAX_SYM_NAME] = {0};
	IMAGEHLP_SYMBOL * pSym;
	unsigned long displacement = 0;
	IMAGEHLP_LINE line_info = {0};
	IMAGEHLP_MODULE module = {0};
	byte __buffer[(sizeof(SYMBOL_INFO) +MAX_SYM_NAME * sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64)];

	if((NULL == dbg_hlp) || (size <= 0) || (NULL == buffer))
	{
		errno = EINVAL;
		return NULL;
	}
	
	strings = (char**)calloc(size,sizeof(char*));
	
	if(NULL == strings)
	{
		errno = ENOMEM;
		return NULL;
	}
	
	pSym = (IMAGEHLP_SYMBOL*)__buffer;

	pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
	pSym->MaxNameLength = MAX_SYM_NAME;


	line_info.SizeOfStruct = sizeof(IMAGEHLP_LINE);
	module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
	
	for(pos = 0; pos < size; pos++)
	{
		stack_frame = (STACKFRAME*)(buffer[pos]);

		if(NULL != stack_frame)
		{
		
			if((*(dbg_hlp->sym_get_sym_from_addr))(dbg_hlp->process_handle,stack_frame->AddrPC.Offset, &displacement,pSym))
			{	
				if((*(dbg_hlp->sym_get_line_from_addr))(dbg_hlp->process_handle,stack_frame->AddrPC.Offset, &displacement,&line_info))
				{
					
						sprintf(str,"**   In %s() at %s:%d", pSym->Name,line_info.FileName,line_info.LineNumber); 
						
						strings[pos] = strdup(str);
						memset(str,0,MAX_SYM_NAME);	
					
						success = 1;
				}
				else
				{
					strings[pos] = strdup("<no line>");
				}
			}
			else
			{
				strings[pos] = strdup("<no symbole>");
			}

			free(stack_frame);
		}
		else
			break;
	}
	
	if(!success)
	{
		free(strings);
		strings = NULL;
	}

	dbg_hlp_finalize();
	
	return strings;
}

void
backtrace_symbols_fd(void *const *buffer, int size, int fd)
{
	int pos;
	int success = 0;	
	STACKFRAME* stack_frame;
	char str[MAX_SYM_NAME] = {0};
	IMAGEHLP_SYMBOL * pSym;
	unsigned long displacement = 0;
	IMAGEHLP_LINE line_info = {0};
	IMAGEHLP_MODULE module = {0};

	byte __buffer[(sizeof(SYMBOL_INFO) +MAX_SYM_NAME * sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64)];

	if((NULL == dbg_hlp) || (size <= 0) || (NULL == buffer) || (-1 == fd))
	{
		errno = EINVAL;
		return;
	}
	
	pSym = (IMAGEHLP_SYMBOL*)__buffer;

	pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
	pSym->MaxNameLength = MAX_SYM_NAME;


	line_info.SizeOfStruct = sizeof(IMAGEHLP_LINE);
	module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
	
	for(pos = 0; pos < size; pos++)
	{
		stack_frame = (STACKFRAME*)(buffer[pos]);

		if(NULL != stack_frame)
		{
		
			if((*(dbg_hlp->sym_get_sym_from_addr))(dbg_hlp->process_handle,stack_frame->AddrPC.Offset, &displacement,pSym))
			{	
				if((*(dbg_hlp->sym_get_line_from_addr))(dbg_hlp->process_handle,stack_frame->AddrPC.Offset, &displacement,&line_info))
				{
					
						
						sprintf(str,"**   In %s() at %s:%d\n", pSym->Name,line_info.FileName,line_info.LineNumber); 
						
						if(-1 == write(fd,str,strlen(str)))
							break;

						memset(str,0,MAX_SYM_NAME);	
					
						success = 1;
				}
			}

			free(stack_frame);
		}
		else
			break;
	}
	
	dbg_hlp_finalize();
	
}

static int
dbg_hlp_init(HANDLE process_handle)
{
	if(dbg_hlp)
	{
		/* debug help is already loaded */
		return 0;
	}

	/* allocation */
	dbg_hlp = (xbt_debug_hlp_t)calloc(1,sizeof(s_xbt_debug_hlp_t));
	
	if(!dbg_hlp)
		return ENOMEM;
	
	/* load the library */
	dbg_hlp->instance = LoadLibraryA("Dbghelp.dll");
	
	if(!(dbg_hlp->instance))
	{
		free(dbg_hlp);
		dbg_hlp = NULL;
		return (int)GetLastError();
	}
	
	/* get the pointers to debug help library exported functions */
	
	if(!((dbg_hlp->sym_initialize) = (xbt_pfn_sym_initialize_t)GetProcAddress(dbg_hlp->instance,"SymInitialize")))
	{
		FreeLibrary(dbg_hlp->instance);
		free(dbg_hlp);
		dbg_hlp = NULL;
		return (int)GetLastError();	
	}
		
	if(!((dbg_hlp->sym_cleanup) = (xbt_pfn_sym_cleanup_t)GetProcAddress(dbg_hlp->instance,"SymCleanup")))
	{
		FreeLibrary(dbg_hlp->instance);
		free(dbg_hlp);
		dbg_hlp = NULL;
		return (int)GetLastError();	
	}
	
	if(!((dbg_hlp->sym_function_table_access) = (xbt_pfn_sym_function_table_access_t)GetProcAddress(dbg_hlp->instance,"SymFunctionTableAccess")))
	{
		FreeLibrary(dbg_hlp->instance);
		free(dbg_hlp);
		dbg_hlp = NULL;
		return (int)GetLastError();	
	}
	
	if(!((dbg_hlp->sym_get_line_from_addr) = (xbt_pfn_sym_get_line_from_addr_t)GetProcAddress(dbg_hlp->instance,"SymGetLineFromAddr")))
	{
		FreeLibrary(dbg_hlp->instance);
		free(dbg_hlp);
		dbg_hlp = NULL;
		return (int)GetLastError();	
	}
	
	if(!((dbg_hlp->sym_get_module_base) = (xbt_pfn_sym_get_module_base_t)GetProcAddress(dbg_hlp->instance,"SymGetModuleBase")))
	{
		FreeLibrary(dbg_hlp->instance);
		free(dbg_hlp);
		dbg_hlp = NULL;
		return (int)GetLastError();	
	}
	
	if(!((dbg_hlp->sym_get_options) = (xbt_pfn_sym_get_options_t)GetProcAddress(dbg_hlp->instance,"SymGetOptions")))
	{
		FreeLibrary(dbg_hlp->instance);
		free(dbg_hlp);
		dbg_hlp = NULL;
		return (int)GetLastError();	
	}
	
	if(!((dbg_hlp->sym_get_sym_from_addr) = (xbt_pfn_sym_get_sym_from_addr_t)GetProcAddress(dbg_hlp->instance,"SymGetSymFromAddr")))
	{
		FreeLibrary(dbg_hlp->instance);
		free(dbg_hlp);
		dbg_hlp = NULL;
		return (int)GetLastError();	
	}
	
	if(!((dbg_hlp->sym_set_options) = (xbt_pfn_sym_set_options_t)GetProcAddress(dbg_hlp->instance,"SymSetOptions")))
	{
		FreeLibrary(dbg_hlp->instance);
		free(dbg_hlp);
		dbg_hlp = NULL;
		return (int)GetLastError();	
	}
	
	if(!((dbg_hlp->stack_walk) = (xbt_pfn_stack_walk_t)GetProcAddress(dbg_hlp->instance,"StackWalk")))
	{
		FreeLibrary(dbg_hlp->instance);
		free(dbg_hlp);
		dbg_hlp = NULL;
		return (int)GetLastError();	
	}
	
	dbg_hlp->process_handle = process_handle;

	(*(dbg_hlp->sym_set_options))((*(dbg_hlp->sym_get_options))() | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);
	
	if(!(*(dbg_hlp->sym_initialize))(dbg_hlp->process_handle,0,1))
	{
		FreeLibrary(dbg_hlp->instance);
		free(dbg_hlp);
		dbg_hlp = NULL;
		return (int)GetLastError();
	}
		
	
	return 0;
}

static int
dbg_hlp_finalize(void)
{
	if(!dbg_hlp)
		return EINVAL;
		
	if(!(*(dbg_hlp->sym_cleanup))(dbg_hlp->process_handle))
		return (int)GetLastError();
	
	if(!FreeLibrary(dbg_hlp->instance))
		return (int)GetLastError();
	
	free(dbg_hlp);
	dbg_hlp = NULL;
	
	return 0;
}
#  endif
#else
/* dummy implementation. We won't use the result, but ex.h needs it to be defined */
int backtrace (void **__array, int __size) {
  return 0;	
}

#endif

#ifdef SIMGRID_TEST
#include <stdio.h>
#include "xbt/ex.h"

XBT_TEST_SUITE("xbt_ex","Exception Handling");

XBT_TEST_UNIT("controlflow",test_controlflow, "basic nested control flow") {
    xbt_ex_t ex;
    volatile int n=1;

    xbt_test_add0("basic nested control flow");

    TRY {
        if (n != 1)
            xbt_test_fail1("M1: n=%d (!= 1)", n);
        n++;
        TRY {
            if (n != 2)
                xbt_test_fail1("M2: n=%d (!= 2)", n);
            n++;
            THROW0(unknown_error,0,"something");
        } CATCH (ex) {
            if (n != 3)
                xbt_test_fail1("M3: n=%d (!= 3)", n);
            n++;
	    xbt_ex_free(ex);
        }
	n++;
        TRY {
            if (n != 5)
                xbt_test_fail1("M2: n=%d (!= 5)", n);
            n++;
            THROW0(unknown_error,0,"something");
        } CATCH (ex) {
            if (n != 6)
                xbt_test_fail1("M3: n=%d (!= 6)", n);
            n++;
            RETHROW;
            n++;
        }
        xbt_test_fail1("MX: n=%d (shouldn't reach this point)", n);
    }
    CATCH(ex) {
        if (n != 7)
            xbt_test_fail1("M4: n=%d (!= 7)", n);
        n++;
        xbt_ex_free(ex);
    }
    if (n != 8)
        xbt_test_fail1("M5: n=%d (!= 8)", n);
}

XBT_TEST_UNIT("value",test_value,"exception value passing") {
    xbt_ex_t ex;

    TRY {
        THROW0(unknown_error, 2, "toto");
    } CATCH(ex) {
        xbt_test_add0("exception value passing");
        if (ex.category != unknown_error)
            xbt_test_fail1("category=%d (!= 1)", ex.category);
        if (ex.value != 2)
            xbt_test_fail1("value=%d (!= 2)", ex.value);
        if (strcmp(ex.msg,"toto"))
            xbt_test_fail1("message=%s (!= toto)", ex.msg);
        xbt_ex_free(ex);
    }
}

XBT_TEST_UNIT("variables",test_variables,"variable value preservation") {
    xbt_ex_t ex;
    int r1, r2;
    volatile int v1, v2;

    r1 = r2 = v1 = v2 = 1234;
    TRY {
        r2 = 5678;
        v2 = 5678;
        THROW0(unknown_error, 0, "toto");
    } CATCH(ex) {
        xbt_test_add0("variable preservation");
        if (r1 != 1234)
            xbt_test_fail1("r1=%d (!= 1234)", r1);
        if (v1 != 1234)
            xbt_test_fail1("v1=%d (!= 1234)", v1);
        /* r2 is allowed to be destroyed because not volatile */
        if (v2 != 5678)
            xbt_test_fail1("v2=%d (!= 5678)", v2);
        xbt_ex_free(ex);
    }
}

XBT_TEST_UNIT("cleanup",test_cleanup,"cleanup handling") {
    xbt_ex_t ex;
    volatile int v1;
    int c;

    xbt_test_add0("cleanup handling");

    v1 = 1234;
    c = 0;
    TRY {
        v1 = 5678;
        THROW0(1, 2, "blah");
    } CLEANUP {
        if (v1 != 5678)
            xbt_test_fail1("v1 = %d (!= 5678)", v1);
        c = 1;
    } CATCH(ex) {
        if (v1 != 5678)
            xbt_test_fail1("v1 = %d (!= 5678)", v1);
        if (!(ex.category == 1 && ex.value == 2 && !strcmp(ex.msg,"blah")))
            xbt_test_fail0("unexpected exception contents");
        xbt_ex_free(ex);
    }
    if (!c)
        xbt_test_fail0("xbt_ex_free not executed");
}


/*
 * The following is the example included in the documentation. It's a good 
 * idea to check its syntax even if we don't try to run it.
 * And actually, it allows to put comments in the code despite doxygen.
 */ 
static char *mallocex(int size) {
  return NULL;
}
#define SMALLAMOUNT 10
#define TOOBIG 100000000

#if 0 /* this contains syntax errors, actually */
static void bad_example(void) {
  struct {char*first;} *globalcontext;
  ex_t ex;

  /* BAD_EXAMPLE */
  TRY {
    char *cp1, *cp2, *cp3;
    
    cp1 = mallocex(SMALLAMOUNT);
    globalcontext->first = cp1;
    cp2 = mallocex(TOOBIG);
    cp3 = mallocex(SMALLAMOUNT);
    strcpy(cp1, "foo");
    strcpy(cp2, "bar");
  } CLEANUP {
    if (cp3 != NULL) free(cp3);
    if (cp2 != NULL) free(cp2);
    if (cp1 != NULL) free(cp1);
  } CATCH(ex) {
    printf("cp3=%s", cp3);
    RETHROW;
  }
  /* end_of_bad_example */
}
#endif
typedef struct {char *first;} global_context_t;
   
static void good_example(void) {
  global_context_t *global_context=malloc(sizeof(global_context_t));
  xbt_ex_t ex;

  /* GOOD_EXAMPLE */
  { /*01*/
    char * volatile /*03*/ cp1 = NULL /*02*/;
    char * volatile /*03*/ cp2 = NULL /*02*/;
    char * volatile /*03*/ cp3 = NULL /*02*/;
    TRY {
      cp1 = mallocex(SMALLAMOUNT);
      global_context->first = cp1;
      cp1 = NULL /*05 give away*/;
      cp2 = mallocex(TOOBIG);
      cp3 = mallocex(SMALLAMOUNT);
      strcpy(cp1, "foo");
      strcpy(cp2, "bar");
    } CLEANUP { /*04*/
      printf("cp3=%s", cp3 == NULL /*02*/ ? "" : cp3);
      if (cp3 != NULL)
	free(cp3);
      if (cp2 != NULL)
	free(cp2);
      /*05 cp1 was given away */
    } CATCH(ex) {
      /*05 global context untouched */
      RETHROW;
    }
  }
  /* end_of_good_example */
}
#endif /* SIMGRID_TEST */
