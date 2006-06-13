/* $Id$ */
/* pmm - parallel matrix multiplication "double diffusion"                  */

/* Copyright (c) 2006 Ahmed Harbaoui.                                       */
/* Copyright (c) 2006 Martin Quinson.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"
#include "xbt/matrix.h"
#define PROC_MATRIX_SIZE 2
#define SLAVE_COUNT (PROC_MATRIX_SIZE*PROC_MATRIX_SIZE)

#define DATA_MATRIX_SIZE 4
const int submatrix_size = DATA_MATRIX_SIZE/PROC_MATRIX_SIZE;

XBT_LOG_NEW_DEFAULT_CATEGORY(pmm,"Parallel Matrix Multiplication");

/* struct for recovering results */
GRAS_DEFINE_TYPE(s_result,struct s_result {
  int linepos;
  int rowpos;
  xbt_matrix_t C GRAS_ANNOTE(subtype,double);
});
typedef struct s_result result_t;

/* struct to send initial data to slave */
GRAS_DEFINE_TYPE(s_assignment,struct s_assignment {
  int linepos;
  int rowpos;
  xbt_host_t line[PROC_MATRIX_SIZE];
  xbt_host_t row[PROC_MATRIX_SIZE];
  xbt_matrix_t A GRAS_ANNOTE(subtype,double);
  xbt_matrix_t B GRAS_ANNOTE(subtype,double);
});
typedef struct s_assignment s_assignment_t;

/* register messages which may be sent (common to client and server) */
static void register_messages(void) {
  gras_datadesc_type_t result_type;
  gras_datadesc_type_t assignment_type;

  gras_datadesc_set_const("PROC_MATRIX_SIZE",PROC_MATRIX_SIZE);
  result_type=gras_datadesc_by_symbol(s_result);
  assignment_type=gras_datadesc_by_symbol(s_assignment);
	
  /* receive a final result from slave */
  gras_msgtype_declare("result", result_type);

  /* send from master to slave to assign a position and some data */
  gras_msgtype_declare("assignment", assignment_type);

  /* send from master to slave to ask a final result */
  gras_msgtype_declare("ask_result", gras_datadesc_by_name("int")); 

  /* send from master to slave to indicate the begining of step */
  gras_msgtype_declare("step", gras_datadesc_by_name("int"));
  /* send from slave to master to indicate the end of the current step */
  gras_msgtype_declare("step_ack", gras_datadesc_by_name("int"));

  /* send data between slave */
  gras_msgtype_declare("dataA", gras_datadesc_by_name("double"));
  /* send data between slave */
  gras_msgtype_declare("dataB", gras_datadesc_by_name("double"));
}

/* Function prototypes */
int slave (int argc,char *argv[]);
int master (int argc,char *argv[]);


/* **********************************************************************
 * master code
 * **********************************************************************/

/* Global private data */
typedef struct {
  int nbr_row,nbr_line;
  int remaining_step;
  int remaining_ack;
} master_data_t;


/***  Function Scatter Sequentiel ***/

static void scatter(){

}/* end_of_Scatter */

/***  Function: Scatter // ***/

static void scatter_parl(){

}/* end_of_Scatter // */

/***  Function: multiplication ***/

static void multiplication(){

}/* end_of_multiplication */

/***  Function: gather ***/

static void gather(){

}/* end_of_gather */

int master (int argc,char *argv[]) {

  xbt_ex_t e;

  int i,port,ask_result,step;

  xbt_matrix_t A,B,C;
  result_t result;

  gras_socket_t from;

  /*  Init the GRAS's infrastructure */
  gras_init(&argc, argv);

  xbt_host_t grid[SLAVE_COUNT]; /* The slaves */
  gras_socket_t socket[SLAVE_COUNT]; /* sockets for brodcast to slaves */

  /*  Initialize Matrices */

  A = xbt_matrix_double_new_id(DATA_MATRIX_SIZE,DATA_MATRIX_SIZE);
  B = xbt_matrix_double_new_seq(DATA_MATRIX_SIZE,DATA_MATRIX_SIZE);
  C = xbt_matrix_double_new_zeros(DATA_MATRIX_SIZE,DATA_MATRIX_SIZE);
	
  //xbt_matrix_dump(B,"B:seq",0,xbt_matrix_dump_display_double);


  /*  Get arguments and create sockets */
  port=atoi(argv[1]);
  //scatter();
  //scatter_parl();
  //multiplication();
  //gather();
  //display(A);
  /************************* Init Data Send *********************************/
  int step_ack;
  gras_os_sleep(5);

  for( i=1;i<argc && i<=SLAVE_COUNT;i++){
    grid[i-1]=xbt_host_from_string(argv[i]);
    socket[i-1]=gras_socket_client(grid[i-1]->name,grid[i-1]->port);
      
    INFO2("Connected to %s:%d.",grid[i-1]->name,grid[i-1]->port);
  }
  /* FIXME: let the surnumerous slave die properly */

  int row=0, line=0;
  for(i=0 ; i<SLAVE_COUNT; i++){
    s_assignment_t assignment;
    int j;

    assignment.linepos=line; // assigned line
    assignment.rowpos=row;   // assigned row

    /* Neiborhood */
    for (j=0; j<PROC_MATRIX_SIZE; j++) {
      assignment.row[j] = grid[ j*PROC_MATRIX_SIZE+(row) ] ;
      assignment.line[j] =  grid[ (line)*PROC_MATRIX_SIZE+j ] ;
    }

    assignment.A=xbt_matrix_new_sub(A,
				    submatrix_size,submatrix_size,
				    submatrix_size*line,submatrix_size*row,
				    NULL);
    assignment.B=xbt_matrix_new_sub(B,
				    submatrix_size,submatrix_size,
				    submatrix_size*line,submatrix_size*row,
				    NULL);
    //xbt_matrix_dump(assignment.B,"assignment.B",0,xbt_matrix_dump_display_double);
    row++;
    if (row >= PROC_MATRIX_SIZE) {
      row=0;
      line++;
    }
		
    gras_msg_send(socket[i],gras_msgtype_by_name("assignment"),&assignment);
    //    INFO3("Send assignment to %s : data A= %.3g & data B= %.3g",
    //	  gras_socket_peer_name(socket[i]),mydata.a,mydata.b);

  }
  // end assignment

  /******************************* multiplication ********************************/
  INFO0("XXXXXXXXXXXXXXXXXXXXXX begin Multiplication");
	
  for (step=0; step < PROC_MATRIX_SIZE; step++){
    for (i=0; i< SLAVE_COUNT; i++){
      TRY {
	gras_msg_send(socket[i], gras_msgtype_by_name("step"), &step);
      } CATCH(e) {
	gras_socket_close(socket[i]);
	RETHROW0("Unable to send the msg : %s");
      }
    }
    INFO1("XXXXXX Next step (%d)",step);

    /* wait for computing and slave messages exchange */

    i=0;	
    while  ( i< SLAVE_COUNT) {
      TRY {
	gras_msg_wait(1300,gras_msgtype_by_name("step_ack"),&from,&step_ack);
      } CATCH(e) {
	RETHROW0("Can't get a Ack step message from slave : %s");
      }
      i++;
      DEBUG3("Got step ack from %s (got %d of %d)",
	    gras_socket_peer_name(from), i, SLAVE_COUNT);
    }
  }
  /*********************************  gather ***************************************/

  ask_result=0;
  for( i=1;i< argc;i++){
    gras_msg_send(socket[i],gras_msgtype_by_name("ask_result"),&ask_result);
    INFO1("Send (Ask Result) message to %s",gras_socket_peer_name(socket[i]));
  }
  /* wait for results */
  for( i=1;i< argc;i++){
    gras_msg_wait(600,gras_msgtype_by_name("result"),&from,&result);
    xbt_matrix_copy_values(result.C,C,   submatrix_size,submatrix_size,
			   submatrix_size*result.linepos,
			   submatrix_size*result.rowpos,
			   0,0,NULL);
  }
  /*    end of gather   */
  INFO0 ("The Result of Multiplication is :");
  xbt_matrix_dump(C,"C:res",0,xbt_matrix_dump_display_double);

  return 0;
} /* end_of_master */

/* **********************************************************************
 * slave code
 * **********************************************************************/

int slave(int argc,char *argv[]) {

  xbt_ex_t e; 

  int step,port,l,result_ack=0; 
  xbt_matrix_t bA=xbt_matrix_new(submatrix_size,submatrix_size,
				 sizeof(double),NULL);
  xbt_matrix_t bB=xbt_matrix_new(submatrix_size,submatrix_size,
				 sizeof(double),NULL);

  int myline,myrow;
  xbt_matrix_t mydataA,mydataB;
  xbt_matrix_t bC=xbt_matrix_double_new_zeros(submatrix_size,submatrix_size);
  
  result_t result;
 
  gras_socket_t from,sock;  /* to exchange data with my neighbor */
  gras_socket_t master;     /* for the barrier */

  /* sockets for brodcast to other slave */
  gras_socket_t socket_line[PROC_MATRIX_SIZE-1];
  gras_socket_t socket_row[PROC_MATRIX_SIZE-1];

  /* Init the GRAS's infrastructure */

  gras_init(&argc, argv);

  /* Get arguments and create sockets */

  port=atoi(argv[1]);
  
  /*  Create my master socket */
  sock = gras_socket_server(port);
  int i;

  /*  Register the known messages */
  register_messages();

  /* Recover my initialized Data and My Position*/
  s_assignment_t assignment;
  INFO2("Launch %s (port=%d); wait for my enrole message",argv[0],port);
  TRY {
    gras_msg_wait(600,gras_msgtype_by_name("assignment"),&master,&assignment);
  } CATCH(e) {
    RETHROW0("Can't get my assignment from master : %s");
  }
  myline  = assignment.linepos;
  myrow   = assignment.rowpos;
  mydataA = assignment.A;
  mydataB = assignment.B;
  INFO1("mydataB=%p",mydataB);

  INFO2("Receive my pos (%d,%d) and assignment",myline,myrow);

  //  xbt_matrix_dump(mydataB,"myB",0,xbt_matrix_dump_display_double);

  /* Get my neighborhood from the assignment message (skipping myself) */
  int j=0;
  for (i=0,j=0 ; i<PROC_MATRIX_SIZE ; i++){
    if (strcmp(gras_os_myname(),assignment.line[i]->name)) {
      socket_line[j]=gras_socket_client(assignment.line[i]->name,
					assignment.line[i]->port);
      j++;
    }
    xbt_host_free(assignment.line[i]);
  }
  for (i=0,j=0 ; i<PROC_MATRIX_SIZE ; i++){
    if (strcmp(gras_os_myname(),assignment.row[i]->name)) {
      socket_row[j]=gras_socket_client(assignment.row[i]->name,
				       assignment.row[i]->port);
      j++;
    }
    xbt_host_free(assignment.row[i]);    
  }

  
  do {  //repeat until compute Cb
	
    TRY {
      gras_msg_wait(200,gras_msgtype_by_name("step"),NULL,&step);
    } CATCH(e) {
      RETHROW0("Can't get a Next Step message from master : %s");
    }
    INFO1("Receive a step message from master: step = %d ",step);

    /* a line brodcast */
    gras_os_sleep(3);  // IL FAUT EXPRIMER LE TEMPS D'ATTENTE EN FONCTION DE "SLAVE_COUNT"

    if(myline==step){
      INFO2("step(%d) = Myline(%d)",step,myline);
      for (l=0;l < PROC_MATRIX_SIZE-1 ;l++){
	INFO1("mydataB=%p",mydataB);
	gras_msg_send(socket_row[l], 
		      gras_msgtype_by_name("dataB"), 
		      &mydataB);
	INFO1("mydataB=%p",mydataB);
	
	xbt_matrix_free(bB);
	INFO1("mydataB=%p",mydataB);
	xbt_matrix_dump(mydataB,"myB",0,xbt_matrix_dump_display_double);
	bB = xbt_matrix_new_sub(mydataB,
				submatrix_size,submatrix_size,
				0,0,NULL);
	
	INFO0("send my data B to my (vertical) neighbors");
      }
    } else {
      INFO2("step(%d) <> Myline(%d)",step,myline);
      TRY {
	INFO1("mydataB=%p",mydataB);
	xbt_matrix_free(bB);
	INFO1("mydataB=%p",mydataB);
	gras_msg_wait(600,gras_msgtype_by_name("dataB"),&from,&bB);
	INFO1("mydataB=%p",mydataB);
      } CATCH(e) {
	RETHROW0("Can't get a data message from line : %s");
      }
      INFO1("Receive data B from my neighbor: %s",
	    gras_socket_peer_name(from));
    }

    /* a row brodcast */
    if (myrow==step) {
      for (l=1;l < PROC_MATRIX_SIZE ;l++){
	gras_msg_send(socket_line[l-1],gras_msgtype_by_name("dataA"), &mydataA);
	xbt_matrix_free(bA);
	bA = xbt_matrix_new_sub(mydataA,
				submatrix_size,submatrix_size,
				0,0,NULL);
	
	INFO0("send my data A to my (horizontal) neighbors");
      }
    } else {
      TRY {
	xbt_matrix_free(bA);
	gras_msg_wait(1200,gras_msgtype_by_name("dataA"), &from,&bA);
      } CATCH(e) {
	RETHROW0("Can't get a data message from row : %s");
      }
      INFO1("Receive data A from my neighbor : %s ",
	    gras_socket_peer_name(from));
    }
    xbt_matrix_double_addmult(bA,bB,bC);

    /* send a ack msg to master */
	
    gras_msg_send(master,gras_msgtype_by_name("step_ack"),&step);
    
    INFO1("Send ack to master for to end %d th step",step);
  	
  } while (step < PROC_MATRIX_SIZE);
  /*  wait Message from master to send the result */
 
  result.C=bC;
  result.linepos=myline;
  result.rowpos=myrow;
 
  TRY {
    gras_msg_wait(600,gras_msgtype_by_name("ask_result"),
		  &master,&result_ack);
  } CATCH(e) {
    RETHROW0("Can't get a data message from line : %s");
  }
  /* send Result to master */
  TRY {
    gras_msg_send(master, gras_msgtype_by_name("result"),&result);
  } CATCH(e) {
    // gras_socket_close(from);
    RETHROW0("Failed to send PING to server: %s");
  }
  INFO2(">>>>>>>> Result sent to %s:%d <<<<<<<<",
	gras_socket_peer_name(master),gras_socket_peer_port(master));
  /*  Free the allocated resources, and shut GRAS down */
  gras_socket_close(master);
  gras_socket_close(from);
  gras_exit();
  INFO0("Done.");
  return 0;
} /* end_of_slave */
