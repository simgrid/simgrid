/* backtrace_windows - backtrace displaying on windows platform             */
/* This file is included by ex.c on need (windows x86)                      */

/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * Win32 (x86) implementation backtrace, backtrace_symbols:
 *  support for application self-debugging.
 */

#if defined(_XBT_BORLAND_COMPILER) || defined(_XBT_VISUALC_COMPILER)
/* native windows build */
#  include <dbghelp.h>
#else
/* gcc-based cross-compiling */
#  include "xbt/wine_dbghelp.h"
#endif


/* SymInitialize() */
typedef BOOL(WINAPI * fun_initialize_t) (HANDLE, PSTR, BOOL);
static fun_initialize_t fun_initialize;

/* SymCleanup() */
typedef BOOL(WINAPI * fun_cleanup_t) (HANDLE hProcess);
static fun_cleanup_t fun_cleanup;

/* SymFunctionTableAccess() */
typedef PVOID(WINAPI * fun_function_table_access_t) (HANDLE, DWORD);
static fun_function_table_access_t fun_function_table_access;

/* SymGetLineFromAddr() */
typedef BOOL(WINAPI * fun_get_line_from_addr_t) (HANDLE, DWORD,
                                                 PDWORD, PIMAGEHLP_LINE);
static fun_get_line_from_addr_t fun_get_line_from_addr;

/* SymGetModuleBase() */
typedef DWORD(WINAPI * fun_get_module_base_t) (HANDLE, DWORD);
static fun_get_module_base_t fun_get_module_base;

/* SymGetOptions() */
typedef DWORD(WINAPI * fun_get_options_t) (VOID);
static fun_get_options_t fun_get_options;

/* SymSetOptions() */
typedef DWORD(WINAPI * fun_set_options_t) (DWORD);
static fun_set_options_t fun_set_options;

/* Pointer function to SymGetSymFromAddr() */
typedef BOOL(WINAPI * fun_get_sym_from_addr_t) (HANDLE, DWORD, PDWORD,
                                                OUT PIMAGEHLP_SYMBOL);
static fun_get_sym_from_addr_t fun_get_sym_from_addr;

/* Pointer function to StackWalk() */
typedef BOOL(WINAPI * fun_stack_walk_t) (DWORD, HANDLE, HANDLE,
                                         LPSTACKFRAME, PVOID,
                                         PREAD_PROCESS_MEMORY_ROUTINE,
                                         PFUNCTION_TABLE_ACCESS_ROUTINE,
                                         PGET_MODULE_BASE_ROUTINE,
                                         PTRANSLATE_ADDRESS_ROUTINE);
static fun_stack_walk_t fun_stack_walk;

static HINSTANCE hlp_dbg_instance = NULL;
static HANDLE process_handle = NULL;


/* Module creation/destruction: initialize our tables */
void xbt_backtrace_preinit(void)
{
  process_handle = GetCurrentProcess();

  if (hlp_dbg_instance) {
    /* debug help is already loaded */
    return;
  }

  /* load the library */
  hlp_dbg_instance = LoadLibraryA("Dbghelp.dll");

  if (!hlp_dbg_instance)
    return;

  /* get the pointers to debug help library exported functions */
  fun_initialize =
    (fun_initialize_t) GetProcAddress(hlp_dbg_instance, "SymInitialize");
  fun_cleanup =
    (fun_cleanup_t) GetProcAddress(hlp_dbg_instance, "SymCleanup");
  fun_function_table_access =
    (fun_function_table_access_t) GetProcAddress(hlp_dbg_instance,
                                                 "SymFunctionTableAccess");
  fun_get_line_from_addr =
    (fun_get_line_from_addr_t) GetProcAddress(hlp_dbg_instance,
                                              "SymGetLineFromAddr");
  fun_get_module_base =
    (fun_get_module_base_t) GetProcAddress(hlp_dbg_instance,
                                           "SymGetModuleBase");
  fun_get_options =
    (fun_get_options_t) GetProcAddress(hlp_dbg_instance, "SymGetOptions");
  fun_get_sym_from_addr =
    (fun_get_sym_from_addr_t) GetProcAddress(hlp_dbg_instance,
                                             "SymGetSymFromAddr");
  fun_set_options =
    (fun_set_options_t) GetProcAddress(hlp_dbg_instance, "SymSetOptions");
  fun_stack_walk =
    (fun_stack_walk_t) GetProcAddress(hlp_dbg_instance, "StackWalk");

  /* Check that everything worked well */
  if (!fun_initialize ||
      !fun_cleanup ||
      !fun_function_table_access ||
      !fun_get_line_from_addr ||
      !fun_get_module_base ||
      !fun_get_options ||
      !fun_get_sym_from_addr || !fun_set_options || !fun_stack_walk) {
    FreeLibrary(hlp_dbg_instance);
    hlp_dbg_instance = NULL;
    return;
  }

  (*fun_set_options) ((*fun_get_options) () |
                      SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);

  if (!(*fun_initialize) (process_handle, 0, 1)) {
    FreeLibrary(hlp_dbg_instance);
    hlp_dbg_instance = NULL;
  }
}

void xbt_backtrace_postexit(void)
{
  if (!hlp_dbg_instance)
    return;

  if ((*fun_cleanup) (process_handle))
    FreeLibrary(hlp_dbg_instance);

  hlp_dbg_instance = NULL;
}

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

int backtrace(void **buffer, int size);

/*
 * backtrace_symbols() function.
 *
 * Given the set of addresses returned by  backtrace()  in  buffer,  back-
 * trace_symbols()  translates the addresses into an array of strings containing
 * the name, the source file and the line number or the las called functions.
 */
char **backtrace_symbols(void *const *buffer, int size);

void xbt_backtrace_current(xbt_ex_t * e)
{
  e->used = backtrace((void **) e->bt, XBT_BACKTRACE_SIZE);
}


void xbt_ex_setup_backtrace(xbt_ex_t * e)
{
  int i;
  char **backtrace_syms;

  xbt_assert0(e
              && e->used,
              "Backtrace not setup yet, cannot set it up for display");

  backtrace_syms = backtrace_symbols(e->bt, e->used);
  e->bt_strings = NULL;
  /* parse the output and build a new backtrace */
  e->bt_strings = xbt_new(char *, e->used);


  for (i = 0; i < e->used; i++)
    e->bt_strings[i] = backtrace_syms[i];

  free(backtrace_syms);
}

int backtrace(void **buffer, int size)
{
  int pos = 0;
  STACKFRAME *stack_frame;
  int first = 1;

  IMAGEHLP_SYMBOL *pSym;
  unsigned long offset = 0;
  IMAGEHLP_LINE line_info = { 0 };
  byte
    __buffer[(sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR) +
              sizeof(ULONG64) - 1) / sizeof(ULONG64)];

  CONTEXT context = { CONTEXT_FULL };
  GetThreadContext(GetCurrentThread(), &context);

  /* ebp points on stack base */
  /* esp points on stack pointer, ie on last stacked element (current element) */
  _asm call $ + 5
    _asm pop eax
    _asm mov context.Eip, eax
    _asm mov eax, esp
    _asm mov context.Esp, eax
    _asm mov context.Ebp, ebp
    if ((NULL == hlp_dbg_instance) || (size <= 0) || (NULL == buffer)) {
    errno = EINVAL;
    return 0;
  }

  for (pos = 0; pos < size; pos++)
    buffer[pos] = NULL;

  pos = 0;

  pSym = (IMAGEHLP_SYMBOL *) __buffer;

  pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
  pSym->MaxNameLength = MAX_SYM_NAME;


  line_info.SizeOfStruct = sizeof(IMAGEHLP_LINE);


  while (pos < size) {
    stack_frame = (void *) xbt_new0(STACKFRAME, 1);

    stack_frame->AddrPC.Offset = context.Eip;
    stack_frame->AddrPC.Mode = AddrModeFlat;

    stack_frame->AddrFrame.Offset = context.Ebp;
    stack_frame->AddrFrame.Mode = AddrModeFlat;

    stack_frame->AddrStack.Offset = context.Esp;
    stack_frame->AddrStack.Mode = AddrModeFlat;

    if ((*fun_stack_walk) (IMAGE_FILE_MACHINE_I386,
                           process_handle,
                           GetCurrentThread(),
                           stack_frame,
                           &context,
                           NULL,
                           fun_function_table_access,
                           fun_get_module_base, NULL)
        && !first) {
      if (stack_frame->AddrReturn.Offset) {

        if ((*fun_get_sym_from_addr)
            (process_handle, stack_frame->AddrPC.Offset, &offset, pSym)) {
          if ((*fun_get_line_from_addr)
              (process_handle, stack_frame->AddrPC.Offset, &offset,
               &line_info))
            buffer[pos++] = (void *) stack_frame;
        }
      } else {
        free(stack_frame);      /* no symbol or no line info */
        break;
      }
    } else {
      free(stack_frame);

      if (first)
        first = 0;
      else
        break;
    }
  }

  return pos;
}

char **backtrace_symbols(void *const *buffer, int size)
{
  int pos;
  int success = 0;
  char **strings;
  STACKFRAME *stack_frame;
  IMAGEHLP_SYMBOL *pSym;
  unsigned long offset = 0;
  IMAGEHLP_LINE line_info = { 0 };
  IMAGEHLP_MODULE module = { 0 };
  byte
    __buffer[(sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR) +
              sizeof(ULONG64) - 1) / sizeof(ULONG64)];

  if ((NULL == hlp_dbg_instance) || (size <= 0) || (NULL == buffer)) {
    errno = EINVAL;
    return NULL;
  }

  strings = xbt_new0(char *, size);

  pSym = (IMAGEHLP_SYMBOL *) __buffer;

  pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
  pSym->MaxNameLength = MAX_SYM_NAME;


  line_info.SizeOfStruct = sizeof(IMAGEHLP_LINE);
  module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

  for (pos = 0; pos < size; pos++) {
    stack_frame = (STACKFRAME *) (buffer[pos]);

    if (NULL != stack_frame) {

      if ((*fun_get_sym_from_addr)
          (process_handle, stack_frame->AddrPC.Offset, &offset, pSym)) {
        if ((*fun_get_line_from_addr)
            (process_handle, stack_frame->AddrPC.Offset, &offset, &line_info)) {
          strings[pos] =
            bprintf("**   In %s() at %s:%d", pSym->Name, line_info.FileName,
                    (int) line_info.LineNumber);
        } else {
          strings[pos] = bprintf("**   In %s()", pSym->Name);
        }
        success = 1;
      } else {
        strings[pos] = xbt_strdup("**   <no symbol>");
      }

      free(stack_frame);
    } else
      break;
  }

  if (!success) {
    free(strings);
    strings = NULL;
  }

  return strings;
}
