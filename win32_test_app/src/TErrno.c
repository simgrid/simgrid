#include <TErrno.h>

/* Global variable */
static errno_t __errno = E_SUCCESS;
static CRITICAL_SECTION errno_cs;
static bool errno_cs_initialized = false;
static is_last_errno = false;

void initializeErrno(void)
{
  if (!errno_cs_initialized) {
    memset(&errno_cs, 0, sizeof(CRITICAL_SECTION));
    InitializeCriticalSection(&errno_cs);
    errno_cs_initialized = true;
  }
}

void terminateErrno(void)
{
  if (errno_cs_initialized) {
    DeleteCriticalSection(&errno_cs);
  }
}


void setErrno(errno_t e)
{
  EnterCriticalSection(&errno_cs);

  if ((E_SUCCESS != e) && !is_last_errno) {
    __errno = e;
    is_last_errno = true;
  }

  LeaveCriticalSection(&errno_cs);
}

errno_t getErrno(void)
{
  errno_t e;
  EnterCriticalSection(&errno_cs);
  e = __errno;
  LeaveCriticalSection(&errno_cs);

  return e;
}
