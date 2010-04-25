/* Broken Peer-To-Peer CAN simulator                                        */

/* Copyright (c) 2006, 2007. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "xbt/sysdep.h"
#include "gras.h"

#include "can_tests.c"						      
//#include "types.h" // header alone containing the typedef struct of a node // include can_tests.c must be OFF.

//XBT_LOG_NEW_DEFAULT_CATEGORY(can,"Messages specific to this example"); // include can_tests.c must be OFF.

//extern char *_gras_this_type_symbol_does_not_exist__s_nuke;
int node_nuke_handler(gras_msg_cb_ctx_t ctx,void *payload_data);

// struct of a "get_successor" message, when a node look after the area in which he want to be.
GRAS_DEFINE_TYPE(s_get_suc,
	struct s_get_suc{
		int xId;
		int yId;
		char host[1024];
		int port;	
	};
);
typedef struct s_get_suc get_suc_t;

// struct of a "response_successor" message, hen a node receive the information of his new area.
GRAS_DEFINE_TYPE(s_rep_suc,
	struct s_rep_suc{
		int x1; // Xmin
		int x2; // Xmax
		int y1; // Ymin
		int y2; // Ymax

		char north_host[1024];
		int north_port;
		char south_host[1024];
		int south_port;
		char east_host[1024];
		int east_port;
		char west_host[1024];
		int west_port;
	};
);
typedef struct s_rep_suc rep_suc_t;

int node(int argc,char **argv);

// registering messages types
static void register_messages(){
	gras_msgtype_declare("can_get_suc",gras_datadesc_by_symbol(s_get_suc));
	gras_msgtype_declare("can_rep_suc",gras_datadesc_by_symbol(s_rep_suc));
	gras_msgtype_declare("can_nuke",gras_datadesc_by_symbol(s_nuke));// can_test.c message // include can_tests.c must be ON.
}


// a forwarding function for a "get_suc" message.
static void forward_get_suc(get_suc_t msg, char host[1024], int port){
	gras_socket_t temp_sock=NULL;
	xbt_ex_t e; // the error variable used in TRY.. CATCH tokens.
	//INFO2("Transmiting message to %s:%d",host,port);	
	TRY{
		temp_sock=gras_socket_client(host,port);
	}CATCH(e){
		RETHROW0("Unable to connect!: %s");
	}
	TRY{
		gras_msg_send(temp_sock,"can_get_suc",&msg);
	}CATCH(e){
		RETHROW0("Unable to send!: %s");
	}
	INFO3("Forwarding a get_successor message to %s for (%d;%d)",host,msg.xId,msg.yId);
	gras_socket_close(temp_sock);
}

// the handling function of a "get_suc" message (what do a node when he receive a "get_suc" message.
static int node_get_suc_handler(gras_msg_cb_ctx_t ctx,void *payload_data){
        gras_socket_t expeditor=gras_msg_cb_ctx_from(ctx);
	get_suc_t *incoming=(get_suc_t*)payload_data;
	xbt_ex_t e; // the error variable used in TRY.. CATCH tokens.
	node_data_t *globals=(node_data_t*)gras_userdata_get();
    	gras_socket_t temp_sock=NULL;
	INFO3("Received a get_successor message from %s for (%d;%d)",gras_socket_peer_name(expeditor),incoming->xId,incoming->yId);
	//INFO4("My area is [%d;%d;%d;%d]",globals->x1,globals->x2,globals->y1,globals->y2);
	if(incoming->xId<globals->x1) // test if the message must be forwarded to a neighbour.
		forward_get_suc(*incoming, globals->west_host, globals->west_port);
	else if(incoming->xId>globals->x2)
		forward_get_suc(*incoming, globals->east_host, globals->east_port);
	else if(incoming->yId<globals->y1)
		forward_get_suc(*incoming, globals->south_host, globals->south_port);
	else if(incoming->yId>globals->y2)
		forward_get_suc(*incoming, globals->north_host, globals->north_port);
	else{ // if the message must not be forwarded, then the area is splitted in half and one half is assignated to the new node.
		rep_suc_t outgoing;
		int validate=0;
		INFO4("Spliting my area between me (%d;%d) and the inserting node (%d;%d)!",
		      globals->xId,globals->yId,incoming->xId,incoming->yId);
		if((globals->x2-globals->x1)>(globals->y2-globals->y1)){ // the height of the area is smaller than its width.
		  if(incoming->xId<globals->xId){ // the new node is west from the actual node.
			outgoing.x1=globals->x1;
			outgoing.x2=(incoming->xId+globals->xId)/2;
			outgoing.y1=globals->y1;
			outgoing.y2=globals->y2;
			strcpy(outgoing.north_host,globals->north_host);
			outgoing.north_port=globals->north_port;
			strcpy(outgoing.south_host,globals->south_host);
			outgoing.south_port=globals->south_port;
			strcpy(outgoing.east_host,globals->host);
			outgoing.east_port=globals->port;
			strcpy(outgoing.west_host,globals->west_host);
			outgoing.west_port=globals->west_port;

			globals->x1=(incoming->xId+globals->xId)/2;
			strcpy(globals->west_host,incoming->host);
			globals->west_port=incoming->port;
			validate=1;
		  }
		  else if(incoming->xId>globals->xId){ // the new node is east from the actual node.
			outgoing.x1=(incoming->xId+globals->xId)/2;
			outgoing.x2=globals->x2;
			outgoing.y1=globals->y1;
			outgoing.y2=globals->y2;
			strcpy(outgoing.north_host,globals->north_host);
			outgoing.north_port=globals->north_port;
			strcpy(outgoing.south_host,globals->south_host);
			outgoing.south_port=globals->south_port;
			strcpy(outgoing.east_host,globals->east_host);
			outgoing.east_port=globals->east_port;
			strcpy(outgoing.west_host,globals->host);
			outgoing.west_port=globals->port;

			globals->x2=(incoming->xId+globals->xId)/2;
			strcpy(globals->east_host,incoming->host);
			globals->east_port=incoming->port;
			validate=1;			
		  }
		}
		else{
		   if(incoming->yId<globals->yId){ // the new node is south from the actual node.
			outgoing.y1=globals->y1;
			outgoing.y2=(incoming->yId+globals->yId)/2;
			outgoing.y1=globals->y1;
			outgoing.x2=globals->x2;
			strcpy(outgoing.east_host,globals->east_host);
			outgoing.east_port=globals->east_port;
			strcpy(outgoing.west_host,globals->west_host);
			outgoing.west_port=globals->west_port;
			strcpy(outgoing.north_host,globals->host);
			outgoing.north_port=globals->port;
			strcpy(outgoing.south_host,globals->south_host);
			outgoing.south_port=globals->south_port;

			globals->y1=(incoming->yId+globals->yId)/2;
			strcpy(globals->south_host,incoming->host);
			globals->south_port=incoming->port;
			validate=1;			
		   }
		   else if(incoming->yId>globals->yId){ // the new node is north from the actual node.
			outgoing.y1=(incoming->yId+globals->yId)/2;
			outgoing.y2=globals->y2;
			outgoing.x1=globals->x1;
			outgoing.x2=globals->x2;
			strcpy(outgoing.east_host,globals->east_host);
			outgoing.east_port=globals->east_port;
			strcpy(outgoing.west_host,globals->west_host);
			outgoing.west_port=globals->west_port;
			strcpy(outgoing.north_host,globals->north_host);
			outgoing.north_port=globals->north_port;
			strcpy(outgoing.south_host,globals->host);
			outgoing.south_port=globals->port;

			globals->y2=(incoming->yId+globals->yId)/2;
			strcpy(globals->north_host,incoming->host);
			globals->north_port=incoming->port;
			validate=1;			
		   }
		}
		if(validate==1){ // the area for the new node has been defined, then send theses informations to the new node.
		        INFO2("Sending environment informations to node %s:%d",incoming->host,incoming->port);

			TRY{
				temp_sock=gras_socket_client(incoming->host,incoming->port);
			}CATCH(e){
				RETHROW0("Unable to connect to the node wich has requested for an area!: %s");
			}
			TRY{
				gras_msg_send(temp_sock,"can_rep_suc",&outgoing);
				INFO0("Environment informations sent!");
			}CATCH(e){
				RETHROW2("%s:Timeout sending environment informations to %s: %s",globals->host,gras_socket_peer_name(expeditor));
			}
			gras_socket_close(temp_sock);
		}
		else // we have a problem!
		        INFO0("An error occurded!!!!!!!!!!!!!");

	}
	gras_socket_close(expeditor); // spare
	TRY{
	  gras_msg_handle(10000.0); // wait a bit in case of someone want to ask me for something.
	}CATCH(e){
	INFO4("My area is [%d;%d;%d;%d]",globals->x1,globals->x2,globals->y1,globals->y2);
	  //INFO0("Closing node, all has been done!");
	}
   return 0;
}





int node(int argc,char **argv){
	node_data_t *globals=NULL;
	xbt_ex_t e; // the error variable used in TRY.. CATCH tokens.
	gras_socket_t temp_sock=NULL;

	rep_suc_t rep_suc_msg;

	get_suc_t get_suc_msg; // building the "get_suc" message.
	gras_socket_t temp_sock2=NULL;

	INFO0("Starting");

	/* 1. Init the GRAS infrastructure and declare my globals */
	gras_init(&argc,argv);
   	gras_os_sleep((15-gras_os_getpid())*20); // wait a bit.
	
	globals=gras_userdata_new(node_data_t);

	globals->xId=atoi(argv[1]); // x coordinate of the node.
	globals->yId=atoi(argv[2]); // y coordinate of the node.
	globals->port=atoi(argv[3]); // node port
	globals->sock=gras_socket_server(globals->port); // node socket.
	snprintf(globals->host,1024,gras_os_myname()); // node name.
	globals->version=0; // node version (used for fun)

	/* 2. Inserting the Node */
	INFO2("Inserting node %s:%d",globals->host,globals->port);
	if(argc==4){ // the node is a server, then he has the whole area.
		globals->x1=0;
		globals->x2=1000;
		globals->y1=0;
		globals->y2=1000;
	}else{ // asking for an area.
		INFO1("Contacting %s so as to request for an area",argv[4]);

		TRY{
			temp_sock=gras_socket_client(argv[4],atoi(argv[5]));
		}CATCH(e){
			RETHROW0("Unable to connect known host to request for an area!: %s");
		}


		get_suc_msg.xId=globals->xId;
		get_suc_msg.yId=globals->yId;
		strcpy(get_suc_msg.host,globals->host);
		get_suc_msg.port=globals->port;
		TRY{ // asking.
			gras_msg_send(temp_sock,"can_get_suc",&get_suc_msg);
		}CATCH(e){
			gras_socket_close(temp_sock);
			RETHROW0("Unable to contact known host to get an area!: %s");
		}
		gras_socket_close(temp_sock);



		TRY{ // waiting for a reply.
			INFO0("Waiting for reply!");
			gras_msg_wait(6000,"can_rep_suc",&temp_sock2,&rep_suc_msg);
		}CATCH(e){
			RETHROW1("%s: Error waiting for an area:%s",globals->host);
		}

		// retreiving the data of the response.
		globals->x1=rep_suc_msg.x1;
		globals->x2=rep_suc_msg.x2;
		globals->y1=rep_suc_msg.y1;
		globals->y2=rep_suc_msg.y2;
		strcpy(globals->north_host,rep_suc_msg.north_host);
		globals->north_port=rep_suc_msg.north_port;
		strcpy(globals->south_host,rep_suc_msg.south_host);
		globals->south_port=rep_suc_msg.south_port;
		strcpy(globals->east_host,rep_suc_msg.east_host);
		globals->east_port=rep_suc_msg.east_port;
		strcpy(globals->west_host,rep_suc_msg.west_host);
		globals->west_port=rep_suc_msg.west_port;

		gras_socket_close(temp_sock); // spare
	}
	INFO2("Node %s:%d inserted",globals->host,globals->port);

	// associating messages to handlers.
	register_messages();
	gras_cb_register("can_get_suc",&node_get_suc_handler);
	gras_cb_register("can_nuke",&node_nuke_handler);// can_test.c handler // include can_tests.c must be ON.
	
	TRY{
	  gras_msg_handle(10000.0); // waiting.. in case of someone has something to say.
	}CATCH(e){
	  	INFO4("My area is [%d;%d;%d;%d]",globals->x1,globals->x2,globals->y1,globals->y2);
	  //INFO0("Closing node, all has been done!");
	}

	gras_socket_close(globals->sock); // spare.
	free(globals); // spare.
	//gras_exit();
	return(0);
}

// END
