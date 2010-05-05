#define _GNU_SOURCE
#include "private.h"
#include <stdlib.h>

memory_map_t get_memory_map(void)
{
  FILE *fp;                  /* File pointer to process's proc maps file */
  char *line = NULL;         /* Temporal storage for each line that is readed */
  ssize_t read;              /* Number of bytes readed */
  size_t n = 0;              /* Amount of bytes to read by getline */
  memory_map_t ret = NULL;   /* The memory map to return */
  
/* The following variables are used during the parsing of the file "maps" */
  s_map_region memreg;       /* temporal map region used for creating the map */                             
  char *lfields[6], *tok, *endptr;
  int i;
  
/* Open the actual process's proc maps file and create the memory_map_t */
/* to be returned. */
  fp = fopen("/proc/self/maps","r");  

  if(!fp)
    xbt_abort();
    
  ret = xbt_new0(s_memory_map_t,1);
  
  /* Read one line at the time, parse it and add it to the memory map to be returned */ 
  while((read = getline(&line, &n, fp)) != -1) {

    /* Wipeout the new line character */
    line[read - 1] = '\0';

    /* Tokenize the line using spaces as delimiters and store each token */
    /* in lfields array. We expect 5 tokens/fields */
    lfields[0] = strtok(line, " ");
    
    for(i=1; i < 6 && lfields[i - 1] != NULL; i++){
      lfields[i] = strtok(NULL, " ");  
    }

    /* Check to see if we got the expected amount of columns */
    if(i < 6)
      xbt_abort();

    /* Ok we are good enough to try to get the info we need */
    /* First get the start and the end address of the map   */
    tok = strtok(lfields[0], "-");   
    if(tok == NULL)
      xbt_abort();
      
    memreg.start_addr = (void *)strtoul(tok, &endptr, 16);
    /* Make sure that the entire string was an hex number */
    if(*endptr != '\0')
      xbt_abort();
          
    tok = strtok(NULL, "-");
    if(tok == NULL)
      xbt_abort();
    
    memreg.end_addr = (void *)strtoul(tok, &endptr, 16);
    /* Make sure that the entire string was an hex number */
    if(*endptr != '\0')
      xbt_abort();
    
    /* Get the permissions flags */
    if(strlen(lfields[1]) < 4)
      xbt_abort();
    
    memreg.perms = 0;
    
    for(i=0; i<3; i++)
      if(lfields[1][i] != '-')
        memreg.perms |= 1 << i;

    if(lfields[1][4] == 'p')
      memreg.perms |= MAP_PRIV;
      
    else if (lfields[1][4] == 's')
      memreg.perms |= MAP_SHARED;     
            
    /* Get the offset value */
    memreg.offset = (void *)strtoul(lfields[2], &endptr, 16);
    /* Make sure that the entire string was an hex number */
    if(*endptr != '\0')
      xbt_abort();
      
    /* Get the device major:minor bytes */
    tok = strtok(lfields[3], ":");
    if(tok == NULL)
      xbt_abort();

    memreg.dev_major = (char)strtoul(tok, &endptr, 16);    
    /* Make sure that the entire string was an hex number */
    if(*endptr != '\0')
      xbt_abort();

    tok = strtok(NULL, ":");
    if(tok == NULL)
      xbt_abort();

    memreg.dev_minor = (char)strtoul(tok, &endptr, 16);    
    /* Make sure that the entire string was an hex number */
    if(*endptr != '\0')
      xbt_abort();
 
    /* Get the inode number and make sure that the entire string was a long int*/
    memreg.inode = strtoul(lfields[4], &endptr, 10);
    if(*endptr != '\0')
      xbt_abort();

    /* And finally get the pathname */
    memreg.pathname = xbt_strdup(lfields[5]);
    
    /* Create space for a new map region in the region's array and copy the */
    /* parsed stuff from the temporal memreg variable */
    ret->regions = xbt_realloc(ret->regions, sizeof(memreg) * (ret->mapsize + 1));
    memcpy(ret->regions + ret->mapsize, &memreg, sizeof(memreg));    
    ret->mapsize++;    
  }
  
  if(line)
    free(line);
  
  return ret;
}
