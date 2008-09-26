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

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(strbuff,xbt,"String buffers");

/**
** Buffer code
**/

void xbt_strbuff_empty(xbt_strbuff_t b) {
  b->used=0;
  b->data[0]='\n';
  b->data[1]='\0';
}
xbt_strbuff_t xbt_strbuff_new(void) {
  xbt_strbuff_t res=malloc(sizeof(s_xbt_strbuff_t));
  res->data=malloc(512);
  res->size=512;
  xbt_strbuff_empty(res);
  return res;
}
void xbt_strbuff_free(xbt_strbuff_t b) {
  if (b) {
    if (b->data)
      free(b->data);
    free(b);
  }
}
void xbt_strbuff_append(xbt_strbuff_t b, const char *toadd) {
  int addlen;
  int needed_space;

  if (!b)
    THROW0(arg_error,0,"Asked to append stuff to NULL buffer");

  addlen = strlen(toadd);
  needed_space=b->used+addlen+1;

  if (needed_space > b->size) {
    b->data = realloc(b->data, MAX(minimal_increment+b->used, needed_space));
    b->size = MAX(minimal_increment+b->used, needed_space);
  }
  strcpy(b->data+b->used, toadd);
  b->used += addlen;
}
void xbt_strbuff_chomp(xbt_strbuff_t b) {
  while (b->data[b->used] == '\n') {
    b->data[b->used] = '\0';
    if (b->used)
      b->used--;
  }
}

void xbt_strbuff_trim(xbt_strbuff_t b) {
  xbt_str_trim(b->data," ");
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
 */
void xbt_strbuff_varsubst(xbt_strbuff_t b, xbt_dict_t patterns) {

  char *beg, *end; /* pointers around the parsed chunk */
  int in_simple_quote=0, in_double_quote=0;
  int done = 0;

  if (b->data[0] == '\0')
    return;
  end = beg = b->data;

  while (!done) {

    switch (*end) {
      case '\\':
        /* Protected char; pass the protection */
        end++;
        if (*end=='\0')
          THROW0(arg_error,0,"String ends with \\");
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
          char *beg_var,*end_var; /* variable name boundary */
          char *beg_subst, *end_subst=NULL; /* where value should be written to */
          char *value, *default_value=NULL;
          int val_len;
          beg_subst = end;


          if (*(++end) == '{') {
            /* the variable name is enclosed in braces. */
            beg_var = end+1;
            /* Search name's end */
            end_var = beg_var;
            while (*end_var != '\0' && *end_var != '}') {
              if (*end_var == ':') {
                /* damn, we have a default value */
                char *p = end_var;
                while (*p != '\0' && *p != '}')
                  p++;
                if (*p == '\0')
                  THROW0(arg_error,0,"Variable default value not terminated ('}' missing)");

                default_value = xbt_malloc(p-end_var);
                memcpy(default_value, end_var+1, p-end_var-1);
                default_value[p-end_var-1] = '\0';

                end_subst = p+1; /* eat '}' */

                break;
              }
              end_var++;
            }
            if (*end_var == '\0')
              THROW0(arg_error,0,"Variable name not terminated ('}' missing)");

            if (!end_subst) /* already set if there's a default value */
              end_subst = end_var+1; /* also kill the } in the name */

            if (end_var == beg_var)
              THROW0(arg_error,0,"Variable name empty (${} is not valid)");


          } else {
            /* name given directly */
            beg_var = end;
            end_var = beg_var;
            while (*end_var != '\0' && *end_var != ' ' && *end_var != '\t' && *end_var != '\n')
              end_var++;
            end_subst = end_var;
            if (end_var == beg_var)
              THROW0(arg_error,0,"Variable name empty ($ is not valid)");
          }
//          DEBUG1("End_var = %s",end_var);

          /* ok, we now have the variable name. Search the dictionary for the substituted value */
          value = xbt_dict_get_or_null_ext(patterns,beg_var,end_var-beg_var);
//          DEBUG4("Search for %.*s, found %s (default value = %s)\n",
//                  end_var-beg_var,beg_var,
//                  (value?value:"NULL"),
//                  (default_value?default_value:"NULL"));

          if (value)
            value = xbt_strdup(value);
          else
            value = xbt_strdup(default_value);

          if (!value)
            value = xbt_strdup("");

          /* En route for the actual substitution */
          val_len = strlen(value);
//          DEBUG2("val_len = %d, key_len=%d",val_len,end_subst-beg_subst);
          if (val_len <= end_subst-beg_subst) {
            /* enough room to do the substitute in place */
//            INFO3("Substitute '%s' with '%s' for %d chars",beg_subst,value, val_len);
            memmove(beg_subst,value,val_len); /* substitute */
//            INFO3("Substitute '%s' with '%s' for %d chars",beg_subst+val_len,end_subst, b->used-(end_subst-b->data)+1);
            memmove(beg_subst+val_len,end_subst, b->used-(end_subst - b->data)+1); /* move the end of the string closer */
            b->used -= end_subst-beg_subst-val_len;  /* update string buffer used size */
          } else {
            /* we have to extend the data area */
            int tooshort = val_len-(end_subst-beg_subst) +1/*don't forget \0*/;
            int newused = b->used + tooshort;
//            DEBUG2("Too short (by %d chars; %d chars left in area)",val_len- (end_subst-beg_subst), b->size - b->used);
            if (newused > b->size) {
              /* We have to realloc the data area before (because b->size is too small). We have to update our pointers, too */
              char *newdata = realloc(b->data, b->used + MAX(minimal_increment,tooshort));
              int offset = newdata - b->data;
              b->data = newdata;
              b->size = b->used + MAX(minimal_increment,tooshort);
              beg_subst += offset;
              end_subst += offset;
            }
            memmove(beg_subst+val_len,end_subst, b->used-(end_subst - b->data)+1); /* move the end of the string a bit further */
            memmove(beg_subst,value,val_len); /* substitute */
            b->used = newused;
          }
          free(value);


          if (default_value)
            free(default_value);
        }
      case '\0':
        return;
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

static void mytest(const char *name, const char *input, const char *patterns, const char *expected) {
  xbt_dynar_t dyn_patterns; /* splited string */
  xbt_dict_t p; /* patterns */
  unsigned int cpt;  char *str; /*foreach*/
  xbt_strbuff_t sb; /* what we test */

  xbt_test_add0(name);
  p=xbt_dict_new();
  dyn_patterns=xbt_str_split(patterns," ");
  xbt_dynar_foreach(dyn_patterns,cpt,str) {
    xbt_dynar_t keyvals = xbt_str_split(str,"=");
    char *key = xbt_dynar_get_as(keyvals,0,char*);
    char *val = xbt_dynar_get_as(keyvals,1,char*);
    xbt_str_subst(key,'_',' ',0); // to put space in names without breaking the enclosing dynar_foreach
    xbt_dict_set(p,key,xbt_strdup(val),free);
    xbt_dynar_free(&keyvals);
  }
  xbt_dynar_free(&dyn_patterns);
  sb = xbt_strbuff_new();
  xbt_strbuff_append(sb,input);
  xbt_strbuff_varsubst(sb, p);
  xbt_dict_free(&p);
  xbt_test_assert4(!strcmp(sb->data,expected),
                   "Input (%s) with patterns (%s) leads to (%s) instead of (%s)",
                   input,patterns,sb->data,expected);
  xbt_strbuff_free(sb);
}

XBT_TEST_SUITE("xbt_strbuff","String Buffers");
XBT_TEST_UNIT("xbt_strbuff_substitute",test_strbuff_substitute, "test the function xbt_strbuff_substitute") {
  mytest("Empty", "", "", "");

  mytest("Value shorter, no braces, only variable", "$tutu", "tutu=t", "t");
  mytest("Value shorter, braces, only variable", "${tutu}", "tutu=t", "t");
  mytest("Value shorter, no braces, data after", "$tutu toto", "tutu=t", "t toto");
  mytest("Value shorter, braces, data after", "${tutu} toto", "tutu=t", "t toto");
  mytest("Value shorter, no braces, data before", "toto $tutu", "tutu=t", "toto t");
  mytest("Value shorter, braces, data before", "toto ${tutu}", "tutu=t", "toto t");
  mytest("Value shorter, no braces, data before and after", "toto $tutu tata", "tutu=t", "toto t tata");
  mytest("Value shorter, braces, data before and after", "toto ${tutu} tata", "tutu=t", "toto t tata");

  mytest("Value as long, no braces, only variable", "$tutu", "tutu=12345", "12345");
  mytest("Value as long, braces, only variable", "${tutu}", "tutu=1234567", "1234567");
  mytest("Value as long, no braces, data after", "$tutu toto", "tutu=12345", "12345 toto");
  mytest("Value as long, braces, data after", "${tutu} toto", "tutu=1234567", "1234567 toto");
  mytest("Value as long, no braces, data before", "toto $tutu", "tutu=12345", "toto 12345");
  mytest("Value as long, braces, data before", "toto ${tutu}", "tutu=1234567", "toto 1234567");
  mytest("Value as long, no braces, data before and after", "toto $tutu tata", "tutu=12345", "toto 12345 tata");
  mytest("Value as long, braces, data before and after", "toto ${tutu} tata", "tutu=1234567", "toto 1234567 tata");

  mytest("Value longer, no braces, only variable", "$t", "t=tututu", "tututu");
  mytest("Value longer, braces, only variable", "${t}", "t=tututu", "tututu");
  mytest("Value longer, no braces, data after", "$t toto", "t=tututu", "tututu toto");
  mytest("Value longer, braces, data after", "${t} toto", "t=tututu", "tututu toto");
  mytest("Value longer, no braces, data before", "toto $t", "t=tututu", "toto tututu");
  mytest("Value longer, braces, data before", "toto ${t}", "t=tututu", "toto tututu");
  mytest("Value longer, no braces, data before and after", "toto $t tata", "t=tututu", "toto tututu tata");
  mytest("Value longer, braces, data before and after", "toto ${t} tata", "t=tututu", "toto tututu tata");

  mytest("Value much longer, no braces, only variable", "$t", "t=" force_resize, force_resize);
  mytest("Value much longer, braces, only variable", "${t}", "t=" force_resize, force_resize);
  mytest("Value much longer, no braces, data after", "$t toto", "t=" force_resize, force_resize " toto");
  mytest("Value much longer, braces, data after", "${t} toto", "t=" force_resize, force_resize " toto");
  mytest("Value much longer, no braces, data before", "toto $t", "t=" force_resize, "toto " force_resize);
  mytest("Value much longer, braces, data before", "toto ${t}", "t=" force_resize, "toto " force_resize);
  mytest("Value much longer, no braces, data before and after", "toto $t tata", "t=" force_resize, "toto " force_resize " tata");
  mytest("Value much longer, braces, data before and after", "toto ${t} tata", "t=" force_resize, "toto " force_resize " tata");

  mytest("Escaped $", "\\$tutu", "tutu=t", "\\$tutu");
  mytest("Space in var name (with braces)", "${tu ti}", "tu_ti=t", "t");

  // Commented: I'm too lazy to do a memmove in var name to remove the backslash after use.
  // Users should use braces.
  //  mytest("Escaped space in var name", "$tu\\ ti", "tu_ti=t", "t");

  mytest("Default value", "${t:toto}", "", "toto");
  mytest("Useless default value (variable already defined)", "${t:toto}", "t=TRUC", "TRUC");

}

#endif /* SIMGRID_TEST */
