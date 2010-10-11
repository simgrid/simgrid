#include <readline.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

long readline(FILE * stream, char **buf, size_t * n)
{
  size_t i;
  int ch;
  int cr = 0;
  fpos_t pos;

  if (!*buf) {
    *buf = calloc(512, sizeof(char));
    *n = 512;
  }

  if (feof(stream))
    return (ssize_t) - 1;

  for (i = 0; (ch = fgetc(stream)) != EOF; i++) {
    if (i >= (*n) + 1)
      *buf = xbt_realloc(*buf, *n += 512);


    (*buf)[i] = ch;

    if (cr && (*buf)[i] != '\n') {
      /* old Mac os uses CR */
      i--;
      (*buf)[i] = '\n';

      /* move to the previous pos (pos of the CR) */
      fsetpos(stream, &pos);

      /* process as linux now */
      cr--;
    }

    if ((*buf)[i] == '\n') {
      if (cr) {
        /* Windows uses CRLF */
        (*buf)[i - 1] = '\n';
        (*buf)[i] = '\0';
        break;
      } else {
        /* Unix use LF */
        i++;
        (*buf)[i] = '\0';
        break;
      }
    } else if (ch == '\r') {
      cr++;

      /* register the CR position for mac */
      fgetpos(stream, &pos);
    }
  }


  if (i == *n)
    *buf = xbt_realloc(*buf, *n += 1);

  /* Mac os file ended with a blank line */
  if (ch == EOF && (*buf)[i - 1] == '\r')
    (*buf)[i - 1] = '\n';

  (*buf)[i] = '\0';

  return (ssize_t) i;
}
