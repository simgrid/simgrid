#ifndef __Stream_H__
#define __STREAM_H__

#include <TTime.h>
#include <TErrno.h>
#include <ctype.h>
#include <windows.h>


/* 
 * Declaration of the s_Stream structure,
 * which represents a file stream.
 */

typedef struct s_Stream {
  FILE *file;                   /* the file stream.                                     */
  char *line;                   /* the current text line                                */
  size_t line_number;           /* line number in the testsuite file    */
  CRITICAL_SECTION cs;          /* std output managment                 */
} s_Stream_t, *Stream_t;

/* Line type. */
typedef enum {
  comment_line_type,            /* the text line is a comment                                                                                   */
  invalid_token_line_type,      /* the text line contains a invalid token                                                               */
  unknwn_meta_command_line_type,        /* the text line contains a unknown macro command                                               */
  invalid_timeout_value_line_type,      /* the text line contains a invalid timeout value                                               */
  timeout_value_line_type,      /* the text line contains a valid timeout value                                                 */
  invalid_exit_code_line_type,  /* the text line contains a invalid exit code value                                             */
  exit_code_line_type,          /* the text line contains a valid exit code value                                               */
  invalid_export_line_type,     /* the text line contains a invalid export meta command                                 */
  export_line_type,             /* the text line contains a valid export meta command                                   */
  invalid_unset_line_type,      /* the text line contains a invalid unset meta command                                  */
  unset_line_type,              /* the text line contains a valid unset meta command                                    */
  enable_output_checking_line_type,     /* the text line contains a enable output checking meta command                 */
  disable_output_checking_line_type,    /* the text line contains a disable output checking meta command                */
  enable_post_output_checking_line_type,        /* the text line contains a enable post output checking meta command    */
  disable_post_output_checking_line_type,       /* the text line contains a disable post output checking meta command   */
  export_failed_line_type,      /* the text line contains a export meta command which failed                    */
  unset_failed_line_type,       /* the text line contains a unset meta command which failed                             */
  create_console_line_type,     /* the text line contains a create console meta command                 */
  create_no_console_line_type,  /* the text line contains a create no console meta command              */
  enable_exit_code_checking_line_type,  /* the text line contains a enable exit code checking                   */
  disable_exit_code_checking_line_type, /* the text line contains a disable exit code checking                  */
  change_directory_line_type,   /* the text line contains a change directory command                   */
  command_line_line_type        /* the text line contains a command line                                */
} line_type_t;

/* 
 * Buffer size used in the getline function. 
 */
#define DEFAULT_ALLOC_SIZE      ((size_t)64)

/* 
 * s_Stream struct connected functions.
 */

/*
 * Create a new s_Stream struct and return
 * a pointer to self
 */

Stream_t Stream_new(void);

/*
 * Returns true if the current text line is blank.
 */
bool Stream_lineIsBlank(Stream_t stream);

/*
 * Return true if the caracter is space or tab.
 */
bool Stream_isBlankChar(char ch);


/*
 * Return E_SUCCESS if the file is valid. 
 * Otherwise the fuction returns E_INVALID_FILE.
 */
errno_t Stream_isValidFile(const char *file_name);


/* 
 * Return E_SUCCESS is the open file operation succeeded.
 * Otherwise the functions returns E_OPEN_FILE_FAILED.
 */
errno_t Stream_openFile(Stream_t ptr, const char *file_name);

/*
 * This function reads an entire line, storing 
 * the address of the buffer containing the  text into  
 * *dest. 
 */
ssize_t Stream_getLine(Stream_t ptr);

/*
 * Return true if the current line is a comment.
 * Otherwise the functions returns false.
 */
bool Stream_lineIsComment(Stream_t stream);

/* Return true if the current line contains a invalide token.
 * Otherwise, the function returns false.
 */
bool Stream_lineContainsInvalidToken(Stream_t stream);

/*
 * Return true if the text line is a meta command.
 * Otherwise, the functions returns false.
 */
bool Stream_lineIsMetaCommand(Stream_t stream);

/* Retun true if the text line contains a unknown meta command.
 * Otherwise the function returns false.
 */
bool Stream_lineIsUnknwnMetaCommand(Stream_t stream);

/*
 * Returns true if the timeout value is invalid.
 * Otherwise the function returns false.
 */
bool Stream_isInvalidTimeout(Stream_t stream);

/*
 * Returns true if the expected code value is invalid.
 * Otherwise the function returns false.
 */
bool Stream_isInvalidExpectedCode(Stream_t stream);

/*
 * Returns true if the export is invalid.
 * Otherwise the function returns false.
 */
bool Stream_isInvalidExport(Stream_t stream);

/*
 * Returns true if the unset is invalid.
 * Otherwise the function returns false.
 */
bool Stream_isInvalidUnset(Stream_t stream);

/* 
 * Return true if the stream line contains a 
 * expected child output. Otherwhise the function
 * returns false.
 */
bool Stream_lineIsExpectedChildOutput(Stream_t stream);

/* 
 * Return true if the stream line contains a 
 * child input. Otherwhise the function
 * returns false.
 */
bool Stream_lineIsChildInput(Stream_t stream);

/*
 * Return true, if the stream line containts a
 * synchrone test case. otherwise the function
 * returns false.
 */
bool Stream_lineIsSyncTestCase(Stream_t stream);

bool Stream_lineIsAsyncTestCase(Stream_t stream);

/*
 * Return true if the text line contains a invalid 
 * meta command. Otherwise the function returns false.
 */
bool Stream_lineIsInvalidMetaCommand(Stream_t stream);

/*
 * Print the file line.
 */
void Stream_printLine(Stream_t stream, line_type_t line_type);

void Stream_lock(Stream_t ptr);
void Stream_unlock(Stream_t ptr);

bool Stream_lineIsChangeDir(Stream_t stream);

extern CRITICAL_SECTION cs;

/* 
 * Free a s_Stream.
 */

void Stream_free(Stream_t ptr);


#endif                          /* #ifndef __STREAM_H__ */
