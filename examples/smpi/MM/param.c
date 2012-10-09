/*!
 * get the parameter specific to the process from a file
 *
 *
 * Authors: Quintin Jean-Noël
 */

#include "topo.h"
#include "param.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(MM_param,
                             "Messages specific for this msg example");

int match(const char *s, char p)
{
  int c = 0;
  while (*s != '\0') {
    if (strncmp(s++, &p, 1)) continue;
    c++;
  }
  return c;
}

char** get_list_param(const char* input){
  if(input==NULL) return NULL;
  char *line = strdup(input);
  int size = match(line, ' ');
  char **list_param = malloc(size*sizeof(char*));
  char *pch = strtok (line," \t\n");
  int i = 0;
  while (pch != NULL)
  {
    if(strcmp(pch,"") != 0 ) {
      list_param[i] = pch;
      i++;
    }
    pch = strtok (NULL, " \t\n");
  }
  return list_param;
}

char** get_conf(MPI_Comm comm, char * filename, int mynoderank)
{
  if(filename == NULL) return NULL;
  if(mynoderank == -1){
    MPI_Comm comm_node;
    split_comm_intra_node(comm, &comm_node, -1);
    MPI_Comm_rank ( comm_node, &mynoderank );
    MPI_Comm_free(&comm_node);
  }
  char name[MPI_MAX_PROCESSOR_NAME];
  int len;
  MPI_Get_processor_name(name, &len);
  FILE* conf;
  conf = fopen(filename, "r");
  if (conf == NULL) {
    XBT_DEBUG(
              "Try to open the configuration file %s\n", filename);
    perror("fopen");
    return NULL;
  }
  char *machine_name = NULL;
  size_t number = 0;
  int err = 1;
  int index = -1;


  /* Try to find the line correponding to this processor */
  while((err = getdelim(&machine_name, &number, ' ', conf)) != -1) {
    while(err == 1){
      err = getdelim(&machine_name, &number, ' ', conf);
    }
    if(err == -1) break;
    XBT_DEBUG(
              "read: %s cmp to %s\n", machine_name, name);
    /* the first element is in machine_name
       it's normally a processor name */
    /* remove the delimiter before doing the comparison*/
    machine_name[strlen(machine_name)-1] = 0;

    if(strcmp(machine_name,name) == 0){
      /* the machine name match */

      /* try to get for which process with the index*/
      char* char_index=NULL;
      err = getdelim(&char_index, &number, ' ', conf);
      while(err == 1){
        err = getdelim(&char_index, &number, ' ', conf);
      }
      if(err == -1) break;

      index=atoi(char_index);
      XBT_DEBUG(
                "read: %d cmp to %d\n",
                index, mynoderank);

      if(index == mynoderank){
        /* we have found the good line
         * we rebuild the line to get every information*/
        char* line = NULL;
        number = 0;
        getline(&line,&number,conf);
        char* line1 = NULL;
        asprintf(&line1,"%s %s %s",name,char_index,line);
        return get_list_param(line1);
      }
    }

    /*prepare for the next line*/
    free(machine_name);
    machine_name = NULL;
    number = 0;
    err = getline(&machine_name, &number,  conf);
    if (err >= 1) {
      free(machine_name);
      machine_name = NULL;
      number = 0;
    }

  }
  XBT_DEBUG(
            "No configuration for %s %d\n", name, mynoderank );
  return NULL;
}


char*** get_conf_all(char * filename, int * nb_process){
  if(filename == NULL) return NULL;

  char *** all_conf = NULL;
  FILE* conf;
  int nb_line = 0;
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;

  conf = fopen(filename, "r");
  if (conf == NULL) {
    XBT_DEBUG(
              "Try to open the configuration file %s\n", filename);
    perror("fopen");
    return NULL;
  }

  while ((linelen = getline(&line, &linecap, conf)) > 0)
    nb_line++;
  fclose(conf);
  conf = fopen(filename, "r");

  all_conf = malloc(sizeof(char**) * nb_line);
  /* Try to find the line correponding to this processor */
  nb_line = 0;
  while ((linelen = getline(&line, &linecap, conf)) > 0){
    if (strcmp(line,"") == 0) continue; //Skip blank line
    if (line[0] == '#') continue; //Skip comment line
    all_conf[nb_line] = get_list_param(line);
    nb_line++;
  }
  if(nb_process != NULL) *nb_process = nb_line;
  return all_conf;
}


void print_conf(MPI_Comm comm, int rank, FILE* file, char * default_options){
  char name[MPI_MAX_PROCESSOR_NAME];
  int len;
  MPI_Get_processor_name(name, &len);
  if(file == NULL) file = stdout;
  if(default_options == NULL) default_options = "";

  MPI_Comm comm_node;
  split_comm_intra_node(comm, &comm_node, 0);

  char ** names = get_names(comm);
  int* index = get_index( comm,  comm_node);
  MPI_Comm_free(&comm_node);
  int size;
  MPI_Comm_size(comm, &size);
  int i=0;
  if(rank == 0){
    fprintf(file, "#processor_name index USER ARGS (like the cpu binding ...)\n");
    for(i = 0; i < size; i++){
      fprintf(file, "%s %d %s\n", names[i],index[i],default_options);
    }
  }
  free(names);
  free(index);
  return;
}
