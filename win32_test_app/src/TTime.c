#include <TTime.h>

void __time(char *t)
{

  time_t timer;
  struct tm *tblock;
  timer = time(NULL);
  tblock = localtime(&timer);

  sprintf(t, "%02d/%02d/%d  %02d:%02d", tblock->tm_mday, tblock->tm_mon,
          tblock->tm_year + 1900, tblock->tm_hour, tblock->tm_min);
}
