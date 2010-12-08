/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <math.h>
#include "msg/msg.h"
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");
#define KEY_BITS  6
#define CHORD_NB_KEYS 64

/*
* Finger Element
*/
typedef struct{
	int id;
	const char *host_name;
	const char *mailbox;
} finger_elem;

/*
 * Node Data
 */
typedef struct{
	int id; // my id
	const char *host_name; //my host name
	const char* mailbox; //my mailbox
	int fingers_nb; // size of finger list
	finger_elem* fingers; // finger list  [ fingers[0] >> Successor
	int next; //next finger to fix
	int pred_id; // predecessor id
	const char* pred_host_name; // predecessor host name
	const char* pred_mailbox; // predecessor mailbox
}data_node;

//global;

static int node(int argc, char *argv[]);
static int sender(int argc,char *argv[]);
static void find_successor_node(data_node *my_data, m_task_t join_task);
static int find_successor(data_node* my_data, int id);
static const char* find_closest_preceding(data_node* n_node, int id); //return a mailbox
static int get_successor_id(m_host_t);

static MSG_error_t test_all(const char *platform_file,
                            const char *application_file);
static void init_finger_table(data_node *data, int known_id);

static int is_in_interval(unsigned int id, unsigned int start, unsigned int end) {

  id = id % CHORD_NB_KEYS;
  start = start % CHORD_NB_KEYS;
  end = end % CHORD_NB_KEYS;

  /* make sure end >= start and id >= start */
  if (end < start) {
    end += CHORD_NB_KEYS;
  }

  if (id < start) {
    id += CHORD_NB_KEYS;
  }

  return id < end;
}

/*
 * Node Function
 */

/*<process host="host_name" function="node">
   <argument value="id"/> <!-- my id -->
   <argument value="mailbox"/> <!-- mailbox -->
   <!-- optional -->
   <argument value="known_host_name" />
   <argument value="knwon_host_mailbox" />
   >argument value="time_to_sleep"/>
  </process>
*/
static int cpt = 0;
int node(int argc, char *argv[])
{
	m_task_t recv_request = NULL;
	int first = 1;
	int joined = 0;
	int res,create = 0;
	if ( argc == 3)   // if no known host are declared >>> first node >>> create chord ring
	{
		create = 1;
	}

	//int id = atoi(argv[1]);
	//int mailbox = atoi(argv[2]);
	// init data node
	data_node *data = xbt_new(data_node,1);
	data->host_name = MSG_host_get_name(MSG_host_self());
	data->id =  atoi(argv[1]);
	data->mailbox = argv[2];
	data->fingers_nb = 1;
	data->fingers = xbt_new(finger_elem,KEY_BITS);
	data->fingers[0].host_name = data->host_name;
	data->fingers[0].id = data->id;
	data->fingers[0].mailbox = data->mailbox;
	data->next = 0;
	data->pred_host_name = NULL;
	data->pred_id = -1;
	data->pred_mailbox = NULL;

/*
 * Ring Point Entry Node
 */
	if (create) // first ring
	{
		INFO0("Create new Chord Ring...");
		joined = 1;
		cpt++;
		//sucessor = n
		data->fingers[0].host_name = data->host_name;
		data->fingers[0].id = data->id;
		data->fingers[0].mailbox = data->mailbox;
		while(cpt < MSG_get_host_number()-1)  //make condition!!!
		{
			recv_request = NULL;
			res = MSG_task_receive(&(recv_request),data->mailbox);
			xbt_assert0(res == MSG_OK, "MSG_receiev failed");
			if (!strcmp(MSG_task_get_name(recv_request), "Join Call"))
			{
				if(MSG_task_get_data(recv_request)==NULL)
				{
					WARN0("Receiving an Empty Data");
				}
				data_node *recv_data = (data_node*)MSG_task_get_data(recv_request);
				INFO1("Receiving a Join Call from %s",recv_data->host_name);
				if (first)
				{
					// predecessor(recv_data) >>>> data
					recv_data->pred_host_name = data->host_name;
					recv_data->pred_id = data->id;
					recv_data->pred_mailbox = data->mailbox;
					data->fingers_nb = 1;
					// successor(recv_data) >>> data
					recv_data->fingers[0].id = data->id;
					recv_data->fingers[0].host_name = data->host_name;
					recv_data->fingers[0].mailbox = data->mailbox;
					//successor(data) >>>> recv_data
					data->fingers[data->fingers_nb - 1].host_name = recv_data->host_name;
					data->fingers[data->fingers_nb - 1].id = recv_data->id;
					data->fingers[data->fingers_nb - 1].mailbox = recv_data->mailbox;
					INFO1("Sending back a Join Request to %s",recv_data->host_name);
					MSG_task_set_name(recv_request,"Join Response");
					MSG_task_send(recv_request,recv_data->mailbox);
					first = 0;
				}
				else{
					find_successor_node(data,recv_request);
				}

			}
		}
	}
/*
 * Joining Node
 */
	else if(!create)
	{
		//Sleep Before Starting
		INFO1("Let's Sleep >>%i",atoi(argv[6]));
		MSG_process_sleep(atoi(argv[5]));
		INFO0("Hey! Let's Send a Join Request");
		//send a join task to the known host via its(known host) mailbox
		const char* known_host_name = argv[3];
		const char* known_mailbox = argv[4];
		int known_id = atoi(argv[5]);
		m_task_t join_request = MSG_task_create("Join Call",10000,2000,data);   // define comp size and comm size (#define ...)
		INFO2("Sending a join request to %s via mailbox %s",known_host_name,known_mailbox);
		MSG_task_send(join_request,known_mailbox);
		//wait for answer on my mailbox
		while(cpt < MSG_get_host_number()-1)
		{
			recv_request = NULL;
			int res = MSG_task_receive(&(recv_request),data->mailbox);
			//check if it's the response for my request
			xbt_assert0(res == MSG_OK, "MSG_receiev failed");
			// get data
			data_node *recv_data = (data_node*)MSG_task_get_data(recv_request);
			// Join Call Message
			if(!strcmp(MSG_task_get_name(recv_request), "Join Call"))
			{

				INFO1("Receiving Join Call From %s",recv_data->host_name);
				if(!joined)
				{
					INFO1("Sorry %s... I'm not yet joined",recv_data->host_name);
					//No Treatment
					MSG_task_set_name(recv_request,"Join Failed");
					MSG_task_send(recv_request,recv_data->mailbox);
						}
					else
					{
						find_successor_node(data,recv_request);
					}

			}
			// Join Response
			else if(!strcmp(MSG_task_get_name(recv_request), "Join Response"))
			{
				INFO0("Receiving Join Response!!!");
				INFO1("My successor is : %s",data->fingers[0].host_name);
				INFO1("My Predecessor is : %s",data->pred_host_name);
				cpt++;
				joined = 1;
				INFO1("My finger table size : %i",data->fingers_nb);
				INFO0("***********************************************************************");

				/*
				MSG_task_set_name(recv_request,"Fix Fingers");

				MSG_task_send(recv_request,data->pred_mailbox);
				MSG_task_send(recv_request,data->fingers[0].mailbox);
				*/
				init_finger_table(data, known_id);

				//treatment
			}
			// Join Failure Message
			else if(!strcmp(MSG_task_get_name(recv_request), "Join Failed"))
			{
				INFO0("My Join call has failed... let's Try Again");
				// send back
				//MSG_task_send(join_request,known_mailbox);
				// !!!!!!!!! YVes Jaques Always...???§§§§**************************

			}
			else if(!strcmp(MSG_task_get_name(recv_request), "Fix Fingers"))
			{
				int i;
				for(i = KEY_BITS -1 ; i>= 0;i--)
				{
					//data->fingers[i] = find_finger_elem(data,(data->id)+pow(2,i-1));
				}
			}
		}
	}
	return 0;
}

/*
 * Initializes 
 */
void init_finger_table(data_node *node, int known_id) {

  // ask known_id who is my immediate successor
//  data->fingers[0].id = remote_find_successor(known_id, data->id + 1);
}

/*
 *
 */
void find_successor_node(data_node* n_data,m_task_t join_task)  //use all data
{
	//get recv data
	data_node *recv_data = (data_node*)MSG_task_get_data(join_task);
	INFO3("recv_data->id : %i , n_data->id :%i , successor->id :%i",recv_data->id,n_data->id,n_data->fingers[0].id);
	//if ((recv_data->id >= n_data->id) && (recv_data->id <= n_data->fingers[0].id))
	if (is_in_interval(recv_data->id,n_data->id,n_data->fingers[0].id))
	{
		INFO1("Successor Is %s",n_data->fingers[0].host_name);
		//predecessor(recv_data) >>>> n_data
		recv_data->pred_host_name = n_data->host_name;
		recv_data->pred_id = n_data->id;
		recv_data->pred_mailbox = n_data->pred_mailbox;
		// successor(recv_data) >>>> n_data.finger[0]
		recv_data->fingers_nb = 1;
		recv_data->fingers[0].host_name = n_data->fingers[0].host_name;
		recv_data->fingers[0].id = n_data->fingers[0].id;
		recv_data->fingers[0].mailbox = n_data->fingers[0].mailbox;
		// successor(n_data) >>>> recv_sucessor
		n_data->fingers[0].id = recv_data->id;
		n_data->fingers[0].host_name = recv_data->host_name;
		n_data->fingers[0].mailbox = recv_data->mailbox;
		// Logs
		INFO1("Sending back a Join Request to %s",recv_data->host_name);
		MSG_task_set_name(join_task,"Join Response");
		MSG_task_send(join_task,recv_data->mailbox);
	}

	else
	{
		const char* closest_preceding_mailbox = find_closest_preceding(n_data,recv_data->id);
		INFO1("Forwarding Join Call to mailbox %s",closest_preceding_mailbox);
		MSG_task_send(join_task,closest_preceding_mailbox);
	}
}

const char* find_closest_preceding(data_node* n_node,int id)
{
	int i;
	for(i = n_node->fingers_nb-1; i >= 0 ; i--)
	{
			if (n_node->fingers[i].id <= id)
				return n_node->fingers[i].mailbox;
	}

	return n_node->mailbox; // !!!!!!!!!!!!!!
}
/*
 * Fin successor id : used to fix finger list
 */
static int find_successor(data_node* n_data, int id)
{
	if (is_in_interval(id,n_data->id,n_data->fingers[0].id))
		return n_data->fingers[0].id;
	else
	    return 0;

}


/** Test function */
MSG_error_t test_all(const char *platform_file,
                     const char *application_file)
{
  MSG_error_t res = MSG_OK;

  /* MSG_config("workstation/model","KCCFLN05"); */
  {                             /*  Simulation setting */
    MSG_set_channel_number(0);
    MSG_create_environment(platform_file);

  }
  {                             /*   Application deployment */
	MSG_function_register("node",node);
    MSG_launch_application(application_file);
  }
  res = MSG_main();
  INFO1("Simulation time %g", MSG_get_clock());

  return res;
}                               /* end_of_test_all */

/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc, argv);
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file\n", argv[0]);
    printf("example: %s msg_platform.xml msg_deployment.xml\n", argv[0]);
    exit(1);
  }
  res = test_all(argv[1], argv[2]);
  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
