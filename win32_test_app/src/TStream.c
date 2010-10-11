#include <TStream.h>

extern const char *TOKENS = "$<>#!p&";

CRITICAL_SECTION cs;

const char *__metacommandlist[] = {
  "set timeout ",
  "enable output checking",
  "disable output checking",
  "enable post output checking",
  "disable post output checking",
  "expect exit code ",
  "export ",
  "unset ",
  "create console",
  "create no console",
  "enable exit code checking",
  "disable exit code checking",
  "command line ",
  NULL
};

/*
 * Create a new s_Stream struct and return
 * a pointer to self
 */

Stream_t Stream_new(void)
{
  Stream_t ptr = (Stream_t) calloc(1, sizeof(s_Stream_t));

  if (NULL == ptr) {
    setErrno(E_STREAM_ALLOCATION_FAILED);
    Stream_free(ptr);
  }

  memset(&cs, 0, sizeof(CRITICAL_SECTION));
  InitializeCriticalSection(&cs);

  ptr->line = NULL;
  ptr->line_number = 0;

  return ptr;
}

/*
 * Returns true if the current text line is blank.
 */
bool Stream_lineIsBlank(Stream_t stream)
{
  size_t i = 0;
  char *p = (char *) stream->line;

  while (p[i] != '\n') {
    if (!Stream_isBlankChar(p[i]))
      return false;
    i++;

  }

  return true;
}

/* 
 * Return true if the caracter is space or tab.
 */
bool Stream_isBlankChar(char ch)
{
  return ((ch == ' ') || ch == ('\t'));
}

/*
 * Return E_SUCCESS if the file is valid. 
 * Otherwise the fuction returns E_INVALID_FILE.
 */
errno_t Stream_isValidFile(const char *file_name)
{
  WIN32_FIND_DATA wfd = { 0 };
  HANDLE hFile = FindFirstFile(file_name, &wfd);

  if (INVALID_HANDLE_VALUE == hFile)
    return E_FILE_NOT_FOUND;

  FindClose(hFile);
  return E_SUCCESS;
}

/* 
 * Return E_SUCCESS is the open file operation succeeded.
 * Otherwise the functions returns E_OPEN_FILE_FAILED.
 */
errno_t Stream_openFile(Stream_t ptr, const char *file_name)
{
  ptr->file = fopen(file_name, "r");

  if (NULL == ptr->file) {
    setErrno(E_OPEN_FILE_FAILED);
    return getErrno();
  }

  return E_SUCCESS;
}

/*
 * This function reads an entire line, storing 
 * the address of the buffer containing the  text into  
 * *dest. 
 */
ssize_t Stream_getLine(Stream_t stream)
{
  size_t capacity_available;    /* capacity available in the buffer                             */
  char *pos;                    /* read operation position                                                      */
  ssize_t size;                 /* the size of the text line (minus the 0 terminal      */
  static size_t len = 0;
  register int ch;              /* the current character                                                        */
  FILE *file = stream->file;

  if (NULL == file) {
    setErrno(E_INVALID_FILE_Stream);
    return -1;
  }

  if (NULL == stream->line) {
    len = DEFAULT_ALLOC_SIZE;
    stream->line = (char *) calloc(1, len);

    if (NULL == stream->line) {
      setErrno(E_STREAM_LINE_ALLOCATION_FAILED);
      return -1;
    }
  } else {
    memset(stream->line, 0, len);
  }

  capacity_available = len;
  pos = stream->line;

  while (true) {
    ch = getc(file);

    /* un byte for the next char and one byte for the zero terminal. */
    if (capacity_available < 2) {
      if (len > DEFAULT_ALLOC_SIZE)
        len = len << 1;
      else
        len += DEFAULT_ALLOC_SIZE;

      capacity_available = stream->line + len - pos;
      stream->line = realloc(stream->line, len);

      if (NULL == stream->line) {
        setErrno(E_STREAM_LINE_REALLOCATION_FAILED);
        return -1;
      }

      pos = stream->line + len - capacity_available;
    }

    if (ferror(file)) {
      /* file error exit on error */
      setErrno(E_STREAM_ERROR);
      return -1;
    }

    if ((EOF == ch)) {
      /* Empty file */
      if (!strlen(stream->line) && !stream->line_number) {
        setErrno(E_STREAM_EMPTY);
        return -1;
      }
      /* end of file */
      else if (!strlen(stream->line) && stream->line_number) {
        return -1;
      }

      break;
    }

    *pos++ = ch;
    capacity_available--;

    /* we have a line, exit loop */
    if (ch == '\n')
      break;
  }

  /* append the zero terminal */

  *pos = '\0';
  size = pos - stream->line;

  stream->line_number++;

  /* size of texte line without zero terminal */
  return size;
}


/* 
 * Free a s_Stream.
 */

void Stream_free(Stream_t ptr)
{
  if (NULL == ptr)
    return;

  if ((NULL != ptr->file) && (stdin != ptr->file))
    fclose(ptr->file);

  if (NULL != ptr->line)
    free(ptr->line);

  DeleteCriticalSection(&cs);

  free(ptr);
}

/*
 * Return true if the current line is a comment.
 * Otherwise the functions returns false.
 */
bool Stream_lineIsComment(Stream_t stream)
{
  return stream->line[0] == '#';
}

/* Return true if the current line contains a invalide token.
 * Otherwise, the function returns false.
 */
bool Stream_lineContainsInvalidToken(Stream_t stream)
{
  if (strchr(TOKENS, stream->line[0]) == NULL) {
    Stream_printLine(stream, invalid_token_line_type);
    setErrno(E_INVALID_TOKEN);
    return true;
  }

  return false;
}

/*
 * Return true if the text line is a meta command.
 * Otherwise, the functions returns false.
 */
bool Stream_lineIsMetacommand(Stream_t stream)
{
  return stream->line[0] == '!';
}

/* Retun true if the text line contains a unknown meta command.
 * Otherwise the function returns false.
 */
bool Stream_lineIsUnknwnMetaCommand(Stream_t stream)
{
  size_t i = 0;
  while (__metacommandlist[i]) {
    if (!strncmp
        (__metacommandlist[i], stream->line + 2,
         strlen(__metacommandlist[i])))
      return false;
    i++;
  }

  Stream_printLine(stream, unknwn_meta_command_line_type);

  setErrno(E_UNKWN_META_COMMAND);
  return true;
}

/*
 * Return true if the text line contains a invalid 
 * meta command. Otherwise the function returns false.
 */
bool Stream_lineIsInvalidMetaCommand(Stream_t stream)
{
  if (!strncmp("set timeout ", stream->line + 2, strlen("set timeout "))) {
    return Stream_isInvalidTimeout(stream);
  } else
      if (!strncmp
          ("command line ", stream->line + 2, strlen("command line "))) {
    Stream_printLine(stream, command_line_line_type);
  } else
      if (!strncmp
          ("enable output checking", stream->line + 2,
           strlen("enable output checking"))) {
    Stream_printLine(stream, enable_output_checking_line_type);
  } else
      if (!strncmp
          ("disable output checking", stream->line + 2,
           strlen("disable output checking"))) {
    Stream_printLine(stream, disable_output_checking_line_type);
  } else
      if (!strncmp
          ("enable post output checking", stream->line + 2,
           strlen("enable post output checking"))) {
    Stream_printLine(stream, enable_post_output_checking_line_type);
  } else
      if (!strncmp
          ("disable post output checking", stream->line + 2,
           strlen("disable post output checking"))) {
    Stream_printLine(stream, disable_post_output_checking_line_type);
  } else
      if (!strncmp
          ("expect exit code ", stream->line + 2,
           strlen("expect exit code "))) {
    return Stream_isInvalidExpectedCode(stream);
  } else if (!strncmp("export ", stream->line + 2, strlen("export "))) {
    return Stream_isInvalidExport(stream);
  } else if (!strncmp("unset ", stream->line + 2, strlen("unset "))) {
    return Stream_isInvalidUnset(stream);
  } else
      if (!strncmp
          ("create console", stream->line + 2, strlen("create console"))) {
    Stream_printLine(stream, create_console_line_type);
  } else
      if (!strncmp
          ("create no console", stream->line + 2,
           strlen("create no console"))) {
    Stream_printLine(stream, create_no_console_line_type);
  } else
      if (!strncmp
          ("enable exit code checking", stream->line + 2,
           strlen("enable exit code checking"))) {
    Stream_printLine(stream, enable_exit_code_checking_line_type);
  } else
      if (!strncmp
          ("disable exit code checking", stream->line + 2,
           strlen("disaable exit code checking"))) {
    Stream_printLine(stream, disable_exit_code_checking_line_type);
  } else {
    return true;
  }

  return false;

}



/*
 * Print the file line.
 */
void Stream_printLine(Stream_t stream, line_type_t line_type)
{
  char *__date = NULL;
  __date = (char *) calloc(1, 30);

  __time(__date);


  Stream_lock(stream);

  switch (line_type) {
#ifdef __VERBOSE
  case comment_line_type:

    if (*(stream->line + 2) != '\0')
      printf("%s   <COMMENT                     >  %3d %s", __date,
             stream->line_number, stream->line + 2);
    else
      /* empty comment */
      printf("%s   <COMMENT                     >  %3d %s", __date,
             stream->line_number, " \n");
    break;

  case timeout_value_line_type:
    printf("%s   <TIMEOUT VALUE IS NOW        >  %3d %s", __date,
           stream->line_number, stream->line + 2 + strlen("set timeout "));
    break;

  case exit_code_line_type:
    printf("%s   <EXPECTED EXIT CODE          >  %3d %s", __date,
           stream->line_number,
           stream->line + 2 + strlen("expect exit code "));
    break;

  case export_line_type:
    printf("%s   <EXPORT                      >  %3d %s", __date,
           stream->line_number, stream->line + 2);
    break;

  case unset_line_type:
    printf("%s   <UNSET                       >  %3d %s", __date,
           stream->line_number, stream->line + 2);
    break;

  case enable_output_checking_line_type:
    printf("%s   <OUTPUT CHECKING ENABLED     >  %3d\n", __date,
           stream->line_number);
    break;

  case disable_output_checking_line_type:
    printf("%s   <OUTPUT CHECKING DISABLED    >  %3d\n", __date,
           stream->line_number);
    break;

  case enable_post_output_checking_line_type:
    printf("%s   <POST OUTPUT CHECKING ENABLED>  %3d\n", __date,
           stream->line_number);
    break;

  case disable_post_output_checking_line_type:
    printf("%s   <POST OUTPUT CHECKING DISABLED>  %3d\n", __date,
           stream->line_number);
    break;

  case create_console_line_type:
    printf("%s   <CREATE CONSOLE SELECTED     >  %3d\n", __date,
           stream->line_number);
    break;

  case create_no_console_line_type:
    printf("%s   <CREATE NO CONSOLE SELECTED  >  %3d\n", __date,
           stream->line_number);
    break;

  case enable_exit_code_checking_line_type:
    printf("%s   <EXIT CODE CHECKING ENABLED  >  %3d\n", __date,
           stream->line_number);
    break;

  case disable_exit_code_checking_line_type:
    printf("%s   <EXIT CODE CHECKING DISABLED >  %3d\n", __date,
           stream->line_number);
    break;

  case change_directory_line_type:
    printf("%s   <DIRECTORY IS NOW            >  %3d %s\n", __date,
           stream->line_number, stream->line + 5);
    break;

  case command_line_line_type:
    printf("%s   <COMMAND LINE                >  %3d %s", __date,
           stream->line_number, stream->line + 2);
    break;

#endif                          /* #ifdef __VERBOSE */

  case invalid_token_line_type:
    printf("%s   <INVALIDE TOKEN              >  %3d %s", __date,
           stream->line_number, stream->line);
    break;

  case unknwn_meta_command_line_type:
    printf("%s   <UNKNOWN META COMMAND        >  %3d %s", __date,
           stream->line_number, stream->line);
    break;

  case invalid_timeout_value_line_type:
    printf("%s   <INVALID TIMEOUT VALUE       >  %3d %s", __date,
           stream->line_number, stream->line);
    break;

  case invalid_exit_code_line_type:
    printf("%s   <INVALID EXIT CODE           >  %3d %s", __date,
           stream->line_number, stream->line);
    break;

  case invalid_export_line_type:
    printf("%s   <INVALID EXPORT              >  %3d %s", __date,
           stream->line_number, stream->line);
    break;

  case invalid_unset_line_type:
    printf("%s   <INVALID UNSET               >  %3d %s", __date,
           stream->line_number, stream->line);
    break;

  case export_failed_line_type:
    printf("%s   <EXPORT FAILED               >  %3d %s", __date,
           stream->line_number, stream->line);
    break;

  case unset_failed_line_type:
    printf("%s   <UNSET FAILED                >  %3d %s", __date,
           stream->line_number, stream->line);
    break;

    /* default:
       ASSERT(false);
     */
  }

  if (__date)
    free(__date);

  Stream_unlock(stream);
}


/*
 * Returns true if the timeout value is invalid.
 * Otherwise the function returns false.
 */
bool Stream_isInvalidTimeout(Stream_t stream)
{
  size_t i = 0;
  char *p = stream->line + 2 + strlen("set timeout ");

  while (p[i] != '\n') {
    if (!isdigit(p[i])) {
      setErrno(E_INVALID_TIMEOUT_VALUE);
      Stream_printLine(stream, invalid_timeout_value_line_type);
      return true;
    }

    i++;
  }

  Stream_printLine(stream, timeout_value_line_type);
  return false;
}


/*
 * Returns true if the expected code value is invalid.
 * Otherwise the function returns false.
 */
bool Stream_isInvalidExpectedCode(Stream_t stream)
{
  size_t i = 0;
  char *p = stream->line + 2 + strlen("expect exit code ");

  while (p[i] != '\n') {
    if (!isdigit(p[i])) {
      setErrno(E_INVALID_EXIT_CODE_VALUE);
      Stream_printLine(stream, invalid_exit_code_line_type);
      return true;
    }
    i++;
  }

  Stream_printLine(stream, exit_code_line_type);
  return false;
}


/*
 * Returns true if the export is invalid.
 * Otherwise the function returns false.
 */
bool Stream_isInvalidExport(Stream_t stream)
{
  /* todo trim */
  const char *ptr = strchr(stream->line, '=');

  if (ptr && (*(++ptr) != '\n')) {
    Stream_printLine(stream, export_line_type);
    return false;
  }

  setErrno(E_INVALID_EXPORT);
  Stream_printLine(stream, invalid_export_line_type);
  return true;
}

/*
 * Returns true if the unset is invalid.
 * Otherwise the function returns false.
 */
bool Stream_isInvalidUnset(Stream_t stream)
{
  /* todo trim */
  const char *ptr = strchr(stream->line, ' ');

  if ((*(++ptr) != '\n')) {
    Stream_printLine(stream, unset_line_type);
    return false;
  }

  setErrno(E_INVALID_UNSET);
  Stream_printLine(stream, invalid_unset_line_type);


  return true;
}


/* 
 * Return true if the stream line contains a 
 * expected child output. Otherwhise the function
 * returns false.
 */

bool Stream_lineIsExpectedChildOutput(Stream_t stream)
{
  return stream->line[0] == '>';
}

/* 
 * Return true if the stream line contains a 
 * child input. Otherwhise the function
 * returns false.
 */
bool Stream_lineIsChildInput(Stream_t stream)
{
  return stream->line[0] == '<';
}



/*
 * Return true, if the stream line containts a
 * synchrone test case. otherwise the function
 * returns false.
 */
bool Stream_lineIsSyncTestCase(Stream_t stream)
{
  return stream->line[0] == '$';
}

bool Stream_lineIsAsyncTestCase(Stream_t stream)
{
  return stream->line[0] == '&';
}

bool Stream_lineIsChangeDir(Stream_t stream)
{
  return ((stream->line[0] == '$')
          && (!strncmp(stream->line + 2, "cd ", strlen("cd "))));
}

void Stream_lock(Stream_t ptr)
{
  EnterCriticalSection(&cs);
}

void Stream_unlock(Stream_t ptr)
{
  LeaveCriticalSection(&cs);
}
