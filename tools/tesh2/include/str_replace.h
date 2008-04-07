#ifndef __STR_REPLACE_H
#define __STR_REPLACE_H

#ifdef __cplusplus
extern "C" {
#endif
int
str_replace(char** str, const char *what, const char *with);

int
str_replace_all(char** str, const char* what, const char* with);

#ifdef __cplusplus
}
#endif

#endif /* !__STR_REPLACE_H */
