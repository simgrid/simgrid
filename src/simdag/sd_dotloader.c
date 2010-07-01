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
#include <graphviz/cgraph.h>

static double parse_double(const char *string) {
	if (string == NULL) return -10;
	int ret = 0;
	double value;

	ret = sscanf(string, "%lg", &value);
	if (ret != 1)
		WARN1("%s is not a double", string);
	return value;
}
static int parse_int(const char *string) {
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
void Start_dot_job(Agnode_t *dag_node) ;
void input_edge(SD_task_t current_job, Agedge_t *edge) ;
void output_edge(SD_task_t current_job, Agedge_t *edge) ;

/** @brief loads a DAX file describing a DAG
 * 
 * See https://confluence.pegasus.isi.edu/display/pegasus/WorkflowGenerator
 * for more details.
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

	xbt_dynar_push(result,&root_task);
	end_task = SD_task_create_comp_seq("end",NULL,0);

	Agnode_t *dag_node   = NULL;
	SD_task_t  task_node  = NULL;
	for (dag_node = agfstnode(dag_dot); dag_node; dag_node = agnxtnode(dag_dot,dag_node)){	
		Start_dot_job(dag_node);	
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

	//INFO2("result : %lld, dot : %lld",result,*dot);
	return result;
}

void Start_dot_job(Agnode_t *dag_node) {
	char *name = agnameof(dag_node);
	double runtime = parse_double(agget(dag_node,"size"));
	int performer = parse_int(agget(dag_node,"performer"));
	INFO3("See <job id=%s runtime=%s %.0f>",name,agget(dag_node,"size"),runtime);
	SD_task_t current_job = SD_task_create_comp_seq(name,(void*)performer,runtime);
	xbt_dict_set(jobs,name,current_job,NULL);
	xbt_dynar_push(result,&current_job);
	Agedge_t    *e;
	int count = 0;
	for (e = agfstin(dag_dot,dag_node); e; e = agnxtin(dag_dot,e)) {
		input_edge(current_job,e);
		count++;
	}
	if (count==0){
		SD_task_t file;
		char *name="root->many";
		double size = 0;

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
	}
	count = 0;
	for (e = agfstout(dag_dot,dag_node); e; e = agnxtout(dag_dot,e)) {
		output_edge(current_job,e);
		count++;
	}
	if (count==0){
		SD_task_t file;
		char *name = "many->end";
		double size = 0;

		//  INFO2("See <uses file=%s %s>",A_dot__uses_file,(is_input?"in":"out"));
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
	}
}

void input_edge(SD_task_t current_job, Agedge_t *edge) {
	SD_task_t file;

	char name[80];
	sprintf(name ,"%s->%s",agnameof(agtail(edge)) ,agnameof(aghead(edge)));
	double size = parse_double(agget(edge,"size"));
	int sender = parse_int(agget(edge,"sender"));
	int reciever = parse_int(agget(edge,"reciever"));

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
}

void output_edge(SD_task_t current_job, Agedge_t *edge) {
	SD_task_t file;
	char name[80];
	sprintf(name ,"%s->%s",agnameof(agtail(edge)) ,agnameof(aghead(edge)));
	double size = parse_double(agget(edge,"size"));
	int sender = parse_int(agget(edge,"sender"));
	int reciever = parse_int(agget(edge,"reciever"));

	//  INFO2("See <uses file=%s %s>",A_dot__uses_file,(is_input?"in":"out"));
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
}

