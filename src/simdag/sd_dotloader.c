/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "simdag/simdag.h"
#include "xbt/misc.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sd_dotparse, sd,"Parsing DOT files");

#undef CLEANUP
#include <cgraph.h>

void dot_add_task(Agnode_t *dag_node) ;
void dot_add_input_dependencies(SD_task_t current_job, Agedge_t *edge) ;
void dot_add_output_dependencies(SD_task_t current_job, Agedge_t *edge) ;
xbt_dynar_t SD_dotload_FILE(FILE* in_file);

static double dot_parse_double(const char *string) {
    if (string == NULL) return -1;
    int ret = 0;
    double value = -1;

    ret = sscanf(string, "%lg", &value);
    if (ret != 1)
        WARN1("%s is not a double", string);
    return value;
}

static int dot_parse_int(const char *string) {
    if (string == NULL) return -10;
    int ret = 0;
    int value;

    ret = sscanf(string, "%d", &value);
    if (ret != 1)
        WARN1("%s is not an integer", string);
    return value;
}

static xbt_dynar_t result;
static xbt_dict_t jobs;
static xbt_dict_t files;
static SD_task_t root_task,end_task;
static Agraph_t * dag_dot;

static void dump_res() {
    unsigned int cursor;
    SD_task_t task;
    xbt_dynar_foreach(result,cursor,task) {
        INFO1("Task %d",cursor);
        SD_task_dump(task);
    }
}

static void dot_task_free(void*task){
    SD_task_t t=task;
    SD_task_destroy(t);
}

/** @brief loads a DOT file describing a DAG
 * 
 * See http://www.graphviz.org/doc/info/lang.html
 * for more details.
 * To obtain information about transfers and tasks, two attributes are
 * required : size on task (execution time in Flop) and size on edge
 * (the amount of data transfer in bit).
 * if they aren't here, there choose to be equal to zero.
 */
xbt_dynar_t SD_dotload(const char*filename){
    FILE* in_file = fopen(filename,"r");
    xbt_assert1(in_file, "Unable to open \"%s\"\n", filename);
    SD_dotload_FILE(in_file);
    fclose(in_file);
    return result;
}

xbt_dynar_t SD_dotload_FILE(FILE* in_file){
    xbt_assert0(in_file, "Unable to use a null file descriptor\n");
    dag_dot = agread(in_file,NIL(Agdisc_t*));

    result = xbt_dynar_new(sizeof(SD_task_t),dot_task_free);
    files=xbt_dict_new();
    jobs=xbt_dict_new();
    root_task = SD_task_create_comp_seq("root",NULL,0);
    /* by design the root task is always SCHEDULABLE */
    __SD_task_set_state(root_task, SD_SCHEDULABLE);

    xbt_dict_set(jobs,"root",root_task,NULL);
    xbt_dynar_push(result,&root_task);
    end_task = SD_task_create_comp_seq("end",NULL,0);
    xbt_dict_set(jobs,"end",end_task,NULL);

    Agnode_t *dag_node   = NULL;
    for (dag_node = agfstnode(dag_dot); dag_node; dag_node = agnxtnode(dag_dot,dag_node)){	
        dot_add_task(dag_node);
    }
    agclose(dag_dot);
    xbt_dict_free(&jobs);

    /* And now, post-process the files.
     * We want a file task per pair of computation tasks exchanging the file. Duplicate on need
     * Files not produced in the system are said to be produced by root task (top of DAG).
     * Files not consumed in the system are said to be consumed by end task (bottom of DAG).
     */
    xbt_dict_cursor_t cursor;
    SD_task_t file;
    char *name;
    xbt_dict_foreach(files,cursor,name,file) {
        unsigned int cpt1,cpt2;
        SD_task_t newfile = NULL;
        SD_dependency_t depbefore,depafter;
        if (xbt_dynar_length(file->tasks_before) == 0) {
            xbt_dynar_foreach(file->tasks_after,cpt2,depafter) {
                SD_task_t newfile = SD_task_create_comm_e2e(file->name,NULL,file->amount);
                SD_task_dependency_add(NULL,NULL,root_task,newfile);
                SD_task_dependency_add(NULL,NULL,newfile,depafter->dst);
                xbt_dynar_push(result,&newfile);
            }
        } else if (xbt_dynar_length(file->tasks_after) == 0) {
            xbt_dynar_foreach(file->tasks_before,cpt2,depbefore) {
                SD_task_t newfile = SD_task_create_comm_e2e(file->name,NULL,file->amount);
                SD_task_dependency_add(NULL,NULL,depbefore->src,newfile);
                SD_task_dependency_add(NULL,NULL,newfile,end_task);
                xbt_dynar_push(result,&newfile);
            }
        } else {
            xbt_dynar_foreach(file->tasks_before,cpt1,depbefore) {
                xbt_dynar_foreach(file->tasks_after,cpt2,depafter) {
                    if (depbefore->src == depafter->dst) {
                        WARN2("File %s is produced and consumed by task %s. This loop dependency will prevent the execution of the task.",
                              file->name,depbefore->src->name);
                    }
                    newfile = SD_task_create_comm_e2e(file->name,NULL,file->amount);
                    SD_task_dependency_add(NULL,NULL,depbefore->src,newfile);
                    SD_task_dependency_add(NULL,NULL,newfile,depafter->dst);
                    xbt_dynar_push(result,&newfile);
                }
            }
        }
    }

    /* Push end task last */
    xbt_dynar_push(result,&end_task);

    /* Free previous copy of the files */
    xbt_dict_free(&files);

    return result;
}

/* dot_add_task create a sd_task and all transfers required for this
 * task. The execution time of the task is given by the attribute size.
 * The unit of size is the Flop.*/
void dot_add_task(Agnode_t *dag_node) {
    char *name = agnameof(dag_node);
    SD_task_t current_job;
    double runtime = dot_parse_double(agget(dag_node,(char*)"size"));
    long performer = (long)dot_parse_int((char *) agget(dag_node,(char*)"performer"));
    INFO3("See <job id=%s runtime=%s %.0f>",name,agget(dag_node,(char*)"size"),runtime);
    current_job = xbt_dict_get_or_null(jobs,name);
    if (current_job == NULL) {
      current_job = SD_task_create_comp_seq(name,(void*)performer,runtime);
      xbt_dict_set(jobs,name,current_job,NULL);
      xbt_dynar_push(result,&current_job);
    }
    Agedge_t    *e;
    int count = 0;
    for (e = agfstin(dag_dot,dag_node); e; e = agnxtin(dag_dot,e)) {
        dot_add_input_dependencies(current_job,e);
        count++;
    }
    if (count==0 && current_job != root_task){
        SD_task_dependency_add(NULL,NULL,root_task,current_job);
    }
    count = 0;
    for (e = agfstout(dag_dot,dag_node); e; e = agnxtout(dag_dot,e)) {
        dot_add_output_dependencies(current_job,e);
        count++;
    }
    if (count==0 && current_job != end_task){
        SD_task_dependency_add(NULL,NULL,current_job,end_task);
    }
}

/* dot_add_output_dependencies create the dependencies between a task
 * and a transfers. This is given by the edges in the dot file. 
 * The amount of data transfers is given by the attribute size on the
 * edge. */
void dot_add_input_dependencies(SD_task_t current_job, Agedge_t *edge) {
    SD_task_t file;

    char name[80];
    sprintf(name ,"%s->%s",agnameof(agtail(edge)) ,agnameof(aghead(edge)));
    double size = dot_parse_double(agget(edge,(char*)"size"));
    INFO2("size : %e, get size : %s",size,agget(edge,(char*)"size"));
    //int sender = dot_parse_int(agget(edge,(char*)"sender"));
    //int reciever = dot_parse_int(agget(edge,(char*)"reciever"));
    if(size >0){
      file = xbt_dict_get_or_null(files,name);
      if (file==NULL) {
        file = SD_task_create_comm_e2e(name,NULL,size);
        xbt_dict_set(files,name,file,&dot_task_free);
      } else {
        if (SD_task_get_amount(file)!=size) {
          WARN3("Ignoring file %s size redefinition from %.0f to %.0f",
                name,SD_task_get_amount(file),size);
        }
      }
      SD_task_dependency_add(NULL,NULL,file,current_job);
    }else{
      file = xbt_dict_get_or_null(jobs,agnameof(agtail(edge)));
      if (file != NULL){
        SD_task_dependency_add(NULL,NULL,file,current_job);
      }
    }
}

/* dot_add_output_dependencies create the dependencies between a
 * transfers and a task. This is given by the edges in the dot file.
 * The amount of data transfers is given by the attribute size on the
 * edge. */
void dot_add_output_dependencies(SD_task_t current_job, Agedge_t *edge) {
    SD_task_t file;
    char name[80];
    sprintf(name ,"%s->%s",agnameof(agtail(edge)) ,agnameof(aghead(edge)));
    double size = dot_parse_double(agget(edge,(char*)"size"));
    INFO2("size : %e, get size : %s",size,agget(edge,(char*)"size"));
    //int sender = dot_parse_int(agget(edge,(char*)"sender"));
    //int reciever = dot_parse_int(agget(edge,(char*)"reciever"));

    //INFO2("See <uses file=%s %s>",A_dot__uses_file,(is_input?"in":"out"));
    if(size >0){
      file = xbt_dict_get_or_null(files,name);
      if (file==NULL) {
        file = SD_task_create_comm_e2e(name,NULL,size);
        xbt_dict_set(files,name,file,&dot_task_free);
      } else {
        if (SD_task_get_amount(file)!=size) {
          WARN3("Ignoring file %s size redefinition from %.0f to %.0f",
                name,SD_task_get_amount(file),size);
        }
      }
      SD_task_dependency_add(NULL,NULL,current_job,file);
      if (xbt_dynar_length(file->tasks_before)>1) {
        WARN1("File %s created at more than one location...",file->name);
      }
    }else{
      file = xbt_dict_get_or_null(jobs,agnameof(aghead(edge)));
      if (file != NULL){
        SD_task_dependency_add(NULL,NULL,current_job,file);
      }
    }
}

