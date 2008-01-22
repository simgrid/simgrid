/* $Id: ex.c 5173 2008-01-07 22:10:52Z mquinson $ */

/* backtrace_windows - backtrace displaying on windows platform             */
/* This file is included by ex.c on need (windows x86)                      */

/*  Copyright (c) 2007 The SimGrid team                                     */
/*  All rights reserved.                                                    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* 
 * Win32 (x86) implementation backtrace, backtrace_symbols
 * : support for application self-debugging.
 */

#include <dbghelp.h>

/* Pointer function to SymInitialize() */
typedef BOOL(WINAPI * xbt_pfn_sym_initialize_t) (HANDLE, PSTR, BOOL);

/* Pointer function to SymCleanup() */
typedef BOOL(WINAPI * xbt_pfn_sym_cleanup_t) (HANDLE hProcess);

/* Pointer function to SymFunctionTableAccess() */
typedef PVOID(WINAPI * xbt_pfn_sym_function_table_access_t) (HANDLE, DWORD);

/* Pointer function to SymGetLineFromAddr() */
typedef BOOL(WINAPI * xbt_pfn_sym_get_line_from_addr_t) (HANDLE, DWORD,
                                                         PDWORD,
                                                         PIMAGEHLP_LINE);

/* Pointer function to SymGetModuleBase() */
typedef DWORD(WINAPI * xbt_pfn_sym_get_module_base_t) (HANDLE, DWORD);

/* Pointer function to SymGetOptions() */
typedef DWORD(WINAPI * xbt_pfn_sym_get_options_t) (VOID);

/* Pointer function to SymGetSymFromAddr() */
typedef BOOL(WINAPI * xbt_pfn_sym_get_sym_from_addr_t) (HANDLE, DWORD, PDWORD,
                                                        OUT PIMAGEHLP_SYMBOL);

/* Pointer function to SymSetOptions() */
typedef DWORD(WINAPI * xbt_pfn_sym_set_options_t) (DWORD);

/* Pointer function to StackWalk() */
typedef BOOL(WINAPI * xbt_pfn_stack_walk_t) (DWORD, HANDLE, HANDLE,
                                             LPSTACKFRAME, PVOID,
                                             PREAD_PROCESS_MEMORY_ROUTINE,
                                             PFUNCTION_TABLE_ACCESS_ROUTINE,
                                             PGET_MODULE_BASE_ROUTINE,
                                             PTRANSLATE_ADDRESS_ROUTINE);

/* This structure represents the debug_help library used interface */
typedef struct s_xbt_debug_help {
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
} s_xbt_debug_hlp_t, *xbt_debug_hlp_t;


/* the address to the unique reference to the debug help library interface */
static xbt_debug_hlp_t dbg_hlp = NULL;

/* initialize the debug help library */
static int dbg_hlp_init(HANDLE process_handle);

/* finalize the debug help library */
static int dbg_hlp_finalize(void);

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

void xbt_ex_setup_backtrace(xbt_ex_t * e)
{
  int i;
  char **backtrace_syms = backtrace_symbols(e->bt, e->used);

  e->used = backtrace((void **) e->bt, XBT_BACKTRACE_SIZE);
  e->bt_strings = NULL;
  /* parse the output and build a new backtrace */
  e->bt_strings = xbt_new(char *, e->used);


  for (i = 0; i < e->used; i++) {
    e->bt_strings[i] = xbt_strdup(backtrace_syms[i]);
    free(backtrace_syms[i]);
  }

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
    _asm mov context.Ebp, ebp dbg_hlp_init(GetCurrentProcess());

  if ((NULL == dbg_hlp) || (size <= 0) || (NULL == buffer)) {
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

    if (!stack_frame) {
      errno = ENOMEM;
      break;
    }

    stack_frame->AddrPC.Offset = context.Eip;
    stack_frame->AddrPC.Mode = AddrModeFlat;

    stack_frame->AddrFrame.Offset = context.Ebp;
    stack_frame->AddrFrame.Mode = AddrModeFlat;

    stack_frame->AddrStack.Offset = context.Esp;
    stack_frame->AddrStack.Mode = AddrModeFlat;

    if ((*(dbg_hlp->stack_walk)) (IMAGE_FILE_MACHINE_I386,
                                  dbg_hlp->process_handle,
                                  GetCurrentThread(),
                                  stack_frame,
                                  &context,
                                  NULL,
                                  dbg_hlp->sym_function_table_access,
                                  dbg_hlp->sym_get_module_base, NULL)
        && !first) {
      if (stack_frame->AddrReturn.Offset) {

        if ((*(dbg_hlp->sym_get_sym_from_addr))
            (dbg_hlp->process_handle, stack_frame->AddrPC.Offset, &offset,
             pSym)) {
          if ((*(dbg_hlp->sym_get_line_from_addr))
              (dbg_hlp->process_handle, stack_frame->AddrPC.Offset, &offset,
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

  if ((NULL == dbg_hlp) || (size <= 0) || (NULL == buffer)) {
    errno = EINVAL;
    return NULL;
  }

  strings = xbt_new0(char *, size);

  if (NULL == strings) {
    errno = ENOMEM;
    return NULL;
  }

  pSym = (IMAGEHLP_SYMBOL *) __buffer;

  pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
  pSym->MaxNameLength = MAX_SYM_NAME;


  line_info.SizeOfStruct = sizeof(IMAGEHLP_LINE);
  module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

  for (pos = 0; pos < size; pos++) {
    stack_frame = (STACKFRAME *) (buffer[pos]);

    if (NULL != stack_frame) {

      if ((*(dbg_hlp->sym_get_sym_from_addr))
          (dbg_hlp->process_handle, stack_frame->AddrPC.Offset, &offset,
           pSym)) {
        if ((*(dbg_hlp->sym_get_line_from_addr))
            (dbg_hlp->process_handle, stack_frame->AddrPC.Offset, &offset,
             &line_info)) {
          strings[pos] =
            bprintf("**   In %s() at %s:%d", pSym->Name, line_info.FileName,
                    line_info.LineNumber);
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

  dbg_hlp_finalize();

  return strings;
}

static int dbg_hlp_init(HANDLE process_handle)
{
  if (dbg_hlp) {
    /* debug help is already loaded */
    return 0;
  }

  /* allocation */
  dbg_hlp = xbt_new0(s_xbt_debug_hlp_t, 1);

  if (!dbg_hlp)
    return ENOMEM;

  /* load the library */
  dbg_hlp->instance = LoadLibraryA("Dbghelp.dll");

  if (!(dbg_hlp->instance)) {
    free(dbg_hlp);
    dbg_hlp = NULL;
    return (int) GetLastError();
  }

  /* get the pointers to debug help library exported functions */

  if (!
      ((dbg_hlp->sym_initialize) =
       (xbt_pfn_sym_initialize_t) GetProcAddress(dbg_hlp->instance,
                                                 "SymInitialize"))) {
    FreeLibrary(dbg_hlp->instance);
    free(dbg_hlp);
    dbg_hlp = NULL;
    return (int) GetLastError();
  }

  if (!
      ((dbg_hlp->sym_cleanup) =
       (xbt_pfn_sym_cleanup_t) GetProcAddress(dbg_hlp->instance,
                                              "SymCleanup"))) {
    FreeLibrary(dbg_hlp->instance);
    free(dbg_hlp);
    dbg_hlp = NULL;
    return (int) GetLastError();
  }

  if (!
      ((dbg_hlp->sym_function_table_access) =
       (xbt_pfn_sym_function_table_access_t) GetProcAddress(dbg_hlp->instance,
                                                            "SymFunctionTableAccess")))
  {
    FreeLibrary(dbg_hlp->instance);
    free(dbg_hlp);
    dbg_hlp = NULL;
    return (int) GetLastError();
  }

  if (!
      ((dbg_hlp->sym_get_line_from_addr) =
       (xbt_pfn_sym_get_line_from_addr_t) GetProcAddress(dbg_hlp->instance,
                                                         "SymGetLineFromAddr")))
  {
    FreeLibrary(dbg_hlp->instance);
    free(dbg_hlp);
    dbg_hlp = NULL;
    return (int) GetLastError();
  }

  if (!
      ((dbg_hlp->sym_get_module_base) =
       (xbt_pfn_sym_get_module_base_t) GetProcAddress(dbg_hlp->instance,
                                                      "SymGetModuleBase"))) {
    FreeLibrary(dbg_hlp->instance);
    free(dbg_hlp);
    dbg_hlp = NULL;
    return (int) GetLastError();
  }

  if (!
      ((dbg_hlp->sym_get_options) =
       (xbt_pfn_sym_get_options_t) GetProcAddress(dbg_hlp->instance,
                                                  "SymGetOptions"))) {
    FreeLibrary(dbg_hlp->instance);
    free(dbg_hlp);
    dbg_hlp = NULL;
    return (int) GetLastError();
  }

  if (!
      ((dbg_hlp->sym_get_sym_from_addr) =
       (xbt_pfn_sym_get_sym_from_addr_t) GetProcAddress(dbg_hlp->instance,
                                                        "SymGetSymFromAddr")))
  {
    FreeLibrary(dbg_hlp->instance);
    free(dbg_hlp);
    dbg_hlp = NULL;
    return (int) GetLastError();
  }

  if (!
      ((dbg_hlp->sym_set_options) =
       (xbt_pfn_sym_set_options_t) GetProcAddress(dbg_hlp->instance,
                                                  "SymSetOptions"))) {
    FreeLibrary(dbg_hlp->instance);
    free(dbg_hlp);
    dbg_hlp = NULL;
    return (int) GetLastError();
  }

  if (!
      ((dbg_hlp->stack_walk) =
       (xbt_pfn_stack_walk_t) GetProcAddress(dbg_hlp->instance,
                                             "StackWalk"))) {
    FreeLibrary(dbg_hlp->instance);
    free(dbg_hlp);
    dbg_hlp = NULL;
    return (int) GetLastError();
  }

  dbg_hlp->process_handle = process_handle;

  (*(dbg_hlp->sym_set_options)) ((*(dbg_hlp->sym_get_options)) () |
                                 SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);

  if (!(*(dbg_hlp->sym_initialize)) (dbg_hlp->process_handle, 0, 1)) {
    FreeLibrary(dbg_hlp->instance);
    free(dbg_hlp);
    dbg_hlp = NULL;
    return (int) GetLastError();
  }


  return 0;
}

static int dbg_hlp_finalize(void)
{
  if (!dbg_hlp)
    return EINVAL;

  if (!(*(dbg_hlp->sym_cleanup)) (dbg_hlp->process_handle))
    return (int) GetLastError();

  if (!FreeLibrary(dbg_hlp->instance))
    return (int) GetLastError();

  free(dbg_hlp);
  dbg_hlp = NULL;

  return 0;
}
