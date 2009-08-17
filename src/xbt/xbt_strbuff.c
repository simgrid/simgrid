/* $Id: buff.c 3483 2007-05-07 11:18:56Z mquinson $ */

/* strbuff -- string buffers                                                */

/* Copyright (c) 2007 Martin Quinson.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* specific to Borland Compiler */
#ifdef __BORLANDDC__
#pragma hdrstop
#endif

#include "xbt/strbuff.h"

#define minimal_increment 512

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(strbuff, xbt, "String buffers");

/**
** Buffer code
**/

void xbt_strbuff_empty(xbt_strbuff_t b)
{
  b->used = 0;
  b->data[0] = '\n';
  b->data[1] = '\0';
}

xbt_strbuff_t xbt_strbuff_new(void)
{
  xbt_strbuff_t res = malloc(sizeof(s_xbt_strbuff_t));
  res->data = malloc(512);
  res->size = 512;
  xbt_strbuff_empty(res);
  return res;
}

/** @brief creates a new string buffer containing the provided string
 *
 * Beware, we store the ctn directly, not a copy of it
 */
xbt_strbuff_t xbt_strbuff_new_from(char *ctn)
{
  xbt_strbuff_t res = malloc(sizeof(s_xbt_strbuff_t));
  res->data = ctn;
  res->used = res->size = strlen(ctn);
  return res;
}

/** @brief frees only the container without touching to the contained string */
void xbt_strbuff_free_container(xbt_strbuff_t b)
{
  free(b);
}

/** @brief frees the buffer and its content */
void xbt_strbuff_free(xbt_strbuff_t b)
{
  if (b) {
    if (b->data)
      free(b->data);
    free(b);
  }
}

void xbt_strbuff_append(xbt_strbuff_t b, const char *toadd)
{
  int addlen;
  int needed_space;

  if (!b)
    THROW0(arg_error, 0, "Asked to append stuff to NULL buffer");

  addlen = strlen(toadd);
  needed_space = b->used + addlen + 1;

  if (needed_space > b->size) {
    b->data =
      realloc(b->data, MAX(minimal_increment + b->used, needed_space));
    b->size = MAX(minimal_increment + b->used, needed_space);
  }
  strcpy(b->data + b->used, toadd);
  b->used += addlen;
}

void xbt_strbuff_chomp(xbt_strbuff_t b)
{
  while (b->data[b->used] == '\n') {
    b->data[b->used] = '\0';
    if (b->used)
      b->used--;
  }
}

void xbt_strbuff_trim(xbt_strbuff_t b)
{
  xbt_str_trim(b->data, " ");
  b->used = strlen(b->data);
}

/** @brief Replaces a set of variables by their values
 *
 * @param b buffer to modify
 * @param patterns variables to substitute in the buffer
 *
 * Both '$toto' and '${toto}' are valid (and the two writing are equivalent).
 *
 * If the variable name contains spaces, use the brace version (ie, ${toto tutu})
 *
 * You can provide a default value to use if the variable is not set in the dict by using
 * '${var:=default}' or '${var:-default}'. These two forms are equivalent, even if they
 * shouldn't to respect the shell standard (:= form should set the value in the dict,
 * but does not) (BUG).
 */
void xbt_strbuff_varsubst(xbt_strbuff_t b, xbt_dict_t patterns)
{

  char *end;                    /* pointers around the parsed chunk */
  int in_simple_quote = 0, in_double_quote = 0;
  int done = 0;

  if (b->data[0] == '\0')
    return;
  end = b->data;

  while (!done) {
    switch (*end) {
    case '\\':
      /* Protected char; pass the protection */
      end++;
      if (*end == '\0')
        THROW0(arg_error, 0, "String ends with \\");
      break;

    case '\'':
      if (!in_double_quote) {
        /* simple quote not protected by double ones, note it */
        in_simple_quote = !in_simple_quote;
      }
      break;
    case '"':
      if (!in_simple_quote) {
        /* double quote protected by simple ones, note it */
        in_double_quote = !in_double_quote;
      }
      break;

    case '$':
      if (!in_simple_quote) {
        /* Go for the substitution. First search the variable name */
        char *beg_var, *end_var;        /* variable name boundary */
        char *beg_subst, *end_subst = NULL;     /* where value should be written to */
        char *value, *default_value = NULL;
        int val_len;
        beg_subst = end;


        if (*(++end) == '{') {
          /* the variable name is enclosed in braces. */
          beg_var = end + 1;
          /* Search name's end */
          end_var = beg_var;
          while (*end_var != '\0' && *end_var != '}') {
            /* TODO: we do not respect the standard for ":=", we should set this value in the dict */
            if (*end_var == ':'
                && ((*(end_var + 1) == '=') || (*(end_var + 1) == '-'))) {
              /* damn, we have a default value */
              char *p = end_var + 1;
              while (*p != '\0' && *p != '}')
                p++;
              if (*p == '\0')
                THROW0(arg_error, 0,
                       "Variable default value not terminated ('}' missing)");

              default_value = xbt_malloc(p - end_var - 1);
              memcpy(default_value, end_var + 2, p - end_var - 2);
              default_value[p - end_var - 2] = '\0';

              end_subst = p + 1;        /* eat '}' */

              break;
            }
            end_var++;
          }
          if (*end_var == '\0')
            THROW0(arg_error, 0,
                   "Variable name not terminated ('}' missing)");

          if (!end_subst)       /* already set if there's a default value */
            end_subst = end_var + 1;    /* also kill the } in the name */

          if (end_var == beg_var)
            THROW0(arg_error, 0, "Variable name empty (${} is not valid)");


        } else {
          /* name given directly */
          beg_var = end;
          end_var = beg_var;
          while (*end_var != '\0' && *end_var != ' ' && *end_var != '\t'
                 && *end_var != '\n')
            end_var++;
          end_subst = end_var;
          if (end_var == beg_var)
            THROW0(arg_error, 0, "Variable name empty ($ is not valid)");
        }
/*        DEBUG5("var='%.*s'; subst='%.*s'; End_var = '%s'",
            end_var-beg_var,beg_var,
            end_subst-beg_subst,beg_subst,
            end_var);*/

        /* ok, we now have the variable name. Search the dictionary for the substituted value */
        value =
          xbt_dict_get_or_null_ext(patterns, beg_var, end_var - beg_var);
/*        DEBUG1("Deal with '%s'",b->data);
        DEBUG4("Search for %.*s, found %s (default value = %s)\n",
            end_var-beg_var,beg_var,
            (value?value:"(no value)"),
            (default_value?default_value:"(no value)"));*/

        if (value)
          value = xbt_strdup(value);
        else if (default_value)
          value = xbt_strdup(default_value);
        else
          value = xbt_strdup("");

        /* En route for the actual substitution */
        val_len = strlen(value);
//        DEBUG2("val_len = %d, key_len=%d",val_len,end_subst-beg_subst);
        if (val_len <= end_subst - beg_subst) {
          /* enough room to do the substitute in place */
//          DEBUG4("Substitute key name by its value: ie '%.*s' by '%.*s'",end_subst-beg_subst,beg_subst,val_len,value);
          memmove(beg_subst, value, val_len);   /* substitute */
//          DEBUG1("String is now: '%s'",b->data);
/*          DEBUG8("Move end of string closer (%d chars moved) :\n-'%.*s%.*s'\n+'%.*s%s'",
              b->used - (end_subst - b->data) + 1,
              beg_subst-b->data,b->data,
              b->used-(end_subst-b->data)+1,beg_subst+val_len,
              beg_subst-b->data,b->data,
              end_subst);*/
          memmove(beg_subst + val_len, end_subst, b->used - (end_subst - b->data) + 1); /* move the end of the string closer */
//          DEBUG1("String is now: '%s'",b->data);
          end = beg_subst + val_len;    /* update the currently explored char in the overall loop */
//          DEBUG1("end of substituted section is now '%s'",end);
          b->used -= end_subst - beg_subst - val_len;   /* update string buffer used size */
//          DEBUG3("Used:%d end:%d ending char:%d",b->used,end-b->data,*end);
        } else {
          /* we have to extend the data area */
          int tooshort =
            val_len - (end_subst - beg_subst) + 1 /*don't forget \0 */ ;
          int newused = b->used + tooshort;
          end += tooshort;      /* update the pointer of the overall loop */
//          DEBUG2("Too short (by %d chars; %d chars left in area)",val_len- (end_subst-beg_subst), b->size - b->used);
          if (newused > b->size) {
            /* We have to realloc the data area before (because b->size is too small). We have to update our pointers, too */
            char *newdata =
              realloc(b->data, b->used + MAX(minimal_increment, tooshort));
            int offset = newdata - b->data;
            b->data = newdata;
            b->size = b->used + MAX(minimal_increment, tooshort);
            end += offset;
            beg_subst += offset;
            end_subst += offset;
          }
          memmove(beg_subst + val_len, end_subst, b->used - (end_subst - b->data) + 2); /* move the end of the string a bit further */
          memmove(beg_subst, value, val_len);   /* substitute */
          b->used = newused;
//          DEBUG1("String is now: %s",b->data);
        }
        free(value);

        if (default_value)
          free(default_value);

        end--; /* compensate the next end++*/
      }
      break;

    case '\0':
      done = 1;
    }
    end++;
  }
}

#ifdef SIMGRID_TEST
#include "xbt/strbuff.h"

/* buffstr have 512 chars by default. Adding 1000 chars like this will force a resize, allowing us to test that b->used and b->size are consistent */
#define force_resize \
  "1.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "2.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "3.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "4.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "5.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "6.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "7.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "8.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "9.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "0.........1.........2.........3.........4.........5.........6.........7.........8.........9........."

static void mytest(const char *input, const char *patterns,
                   const char *expected)
{
  xbt_dynar_t dyn_patterns;     /* splited string */
  xbt_dict_t p;                 /* patterns */
  unsigned int cpt;
  char *str;                    /*foreach */
  xbt_strbuff_t sb;             /* what we test */

  p = xbt_dict_new();
  dyn_patterns = xbt_str_split(patterns, " ");
  xbt_dynar_foreach(dyn_patterns, cpt, str) {
    xbt_dynar_t keyvals = xbt_str_split(str, "=");
    char *key = xbt_dynar_get_as(keyvals, 0, char *);
    char *val = xbt_dynar_get_as(keyvals, 1, char *);
    xbt_str_subst(key, '_', ' ', 0);    // to put space in names without breaking the enclosing dynar_foreach
    xbt_dict_set(p, key, xbt_strdup(val), free);
    xbt_dynar_free(&keyvals);
  }
  xbt_dynar_free(&dyn_patterns);
  sb = xbt_strbuff_new();
  xbt_strbuff_append(sb, input);
  xbt_strbuff_varsubst(sb, p);
  xbt_dict_free(&p);
  xbt_test_assert4(!strcmp(sb->data, expected),
                   "Input (%s) with patterns (%s) leads to (%s) instead of (%s)",
                   input, patterns, sb->data, expected);
  xbt_strbuff_free(sb);
}

XBT_TEST_SUITE("xbt_strbuff", "String Buffers");
XBT_TEST_UNIT("xbt_strbuff_substitute", test_strbuff_substitute,"test the function xbt_strbuff_substitute")
{
  xbt_test_add0("Empty");
  mytest("", "", "");

  xbt_test_add0("Value shorter, no braces, only variable");
  mytest("$tutu", "tutu=t", "t");
  xbt_test_add0("Value shorter, braces, only variable");
  mytest("${tutu}", "tutu=t", "t");
  xbt_test_add0("Value shorter, no braces, data after");
  mytest("$tutu toto", "tutu=t", "t toto");
  xbt_test_add0("Value shorter, braces, data after");
  mytest("${tutu} toto", "tutu=t", "t toto");
  xbt_test_add0("Value shorter, no braces, data before");
  mytest("toto $tutu", "tutu=t", "toto t");
  xbt_test_add0("Value shorter, braces, data before");
  mytest("toto ${tutu}", "tutu=t", "toto t");
  xbt_test_add0("Value shorter, no braces, data before and after");
  mytest("toto $tutu tata", "tutu=t", "toto t tata");
  xbt_test_add0("Value shorter, braces, data before and after");
  mytest("toto ${tutu} tata", "tutu=t", "toto t tata");

  xbt_test_add0("Value as long, no braces, only variable");
  mytest("$tutu", "tutu=12345", "12345");
  xbt_test_add0("Value as long, braces, only variable");
  mytest("${tutu}", "tutu=1234567", "1234567");
  xbt_test_add0("Value as long, no braces, data after");
  mytest("$tutu toto", "tutu=12345", "12345 toto");
  xbt_test_add0("Value as long, braces, data after");
  mytest("${tutu} toto", "tutu=1234567", "1234567 toto");
  xbt_test_add0("Value as long, no braces, data before");
  mytest("toto $tutu", "tutu=12345", "toto 12345");
  xbt_test_add0("Value as long, braces, data before");
  mytest("toto ${tutu}", "tutu=1234567", "toto 1234567");
  xbt_test_add0("Value as long, no braces, data before and after");
  mytest("toto $tutu tata", "tutu=12345", "toto 12345 tata");
  xbt_test_add0("Value as long, braces, data before and after");
  mytest("toto ${tutu} tata", "tutu=1234567", "toto 1234567 tata");

  xbt_test_add0("Value longer, no braces, only variable");
  mytest("$t", "t=tututu", "tututu");
  xbt_test_add0("Value longer, braces, only variable");
  mytest("${t}", "t=tututu", "tututu");
  xbt_test_add0("Value longer, no braces, data after");
  mytest("$t toto", "t=tututu", "tututu toto");
  xbt_test_add0("Value longer, braces, data after");
  mytest("${t} toto", "t=tututu", "tututu toto");
  xbt_test_add0("Value longer, no braces, data before");
  mytest("toto $t", "t=tututu", "toto tututu");
  xbt_test_add0("Value longer, braces, data before");
  mytest("toto ${t}", "t=tututu", "toto tututu");
  xbt_test_add0("Value longer, no braces, data before and after");
  mytest("toto $t tata", "t=tututu", "toto tututu tata");
  xbt_test_add0("Value longer, braces, data before and after");
  mytest("toto ${t} tata", "t=tututu", "toto tututu tata");

  xbt_test_add0("Value much longer, no braces, only variable");
  mytest("$t", "t=" force_resize, force_resize);
  xbt_test_add0("Value much longer, no braces, data after");
  mytest("$t toto", "t=" force_resize, force_resize " toto");
  xbt_test_add0("Value much longer, braces, data after");
  mytest("${t} toto", "t=" force_resize, force_resize " toto");
  xbt_test_add0("Value much longer, no braces, data before");
  mytest("toto $t", "t=" force_resize, "toto " force_resize);
  xbt_test_add0("Value much longer, braces, data before");
  mytest("toto ${t}", "t=" force_resize, "toto " force_resize);
  xbt_test_add0("Value much longer, no braces, data before and after");
  mytest("toto $t tata", "t=" force_resize, "toto " force_resize " tata");
  xbt_test_add0("Value much longer, braces, data before and after");
  mytest("toto ${t} tata", "t=" force_resize, "toto " force_resize " tata");

  xbt_test_add0("Escaped $");
  mytest("\\$tutu", "tutu=t", "\\$tutu");
  xbt_test_add0("Space in var name (with braces)");
  mytest("${tu ti}", "tu_ti=t", "t");

  xbt_test_add0("Two variables");
  mytest("$toto $tutu", "toto=1 tutu=2", "1 2");

  // Commented: I'm too lazy to do a memmove in var name to remove the backslash after use.
  // Users should use braces.
  //  xbt_test_add0("Escaped space in var name", "$tu\\ ti", "tu_ti=t", "t");

  xbt_test_add0("Default value");
  mytest("${t:-toto}", "", "toto");
  xbt_test_add0("Useless default value (variable already defined)");
  mytest("${t:-toto}", "t=TRUC", "TRUC");

}

#endif /* SIMGRID_TEST */
