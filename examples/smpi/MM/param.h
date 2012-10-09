#ifndef param_H_
#define param_H_

#include <stdio.h>

/*!
 * \page param Provide specific parameters to some processes
 * List of functions:
 * - get_conf - reads a file and returns the specific parameters for the process
 * - print_conf - prints the file for the platform used for this execution
 *
 * here an examples of file:
 *
 * \include utils/param_template_file.txt
 *
 * each process will get the list of arguments specific to its.
 *
 * for example, the process 3<sup>th</sup> on the node1 will receive a pointer
 * on an array of char*:
 *
 *
 *  "node1" "2" "node1_arg21" "node1_arg22" "node1_arg2"
 */

/*! reads a file and returns the specific parameters for the process */
char** get_conf(MPI_Comm comm, const char * filename, int mynoderank);

/*! reads a file and returns the parameters of every processes */
char*** get_conf_all(char * filename, int * nb_process);


/*! prints the file for the platform used for this execution */
void print_conf(MPI_Comm comm, int rank, FILE* file, char * default_options);

#endif /*param_H_*/
