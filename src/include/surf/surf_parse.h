/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURF_PARSE_H
#define _SURF_SURF_PARSE_H

typedef enum {
  TOKEN_EMPTY = 0,
  TOKEN_LP = 512,
  TOKEN_RP,
  TOKEN_BEGIN_SECTION,
  TOKEN_END_SECTION,
  TOKEN_CLOSURE,
  TOKEN_WORD,
  TOKEN_NEWLINE,
  TOKEN_ERROR
} e_surf_token_t;

#define MAX_STR_CONST 1024

extern char *surf_parse_text;
extern int line_pos;
extern int char_pos;
extern int tok_num;

e_surf_token_t surf_parse(void);
void  surf_parse_open(const char *file);
void  surf_parse_close(void);

#endif
