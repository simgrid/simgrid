/* $Id$ */
/* pmm - parallel matrix multiplication "double diffusion"                  */

/* Copyright (c) 2006- Ahmed Harbaoui. All rights reserved.                 */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"
#define PROC_MATRIX_SIZE 3
#define SLAVE_COUNT (PROC_MATRIX_SIZE*PROC_MATRIX_SIZE)

#define DATA_MATRIX_SIZE 3

XBT_LOG_NEW_DEFAULT_CATEGORY(pmm,"Parallel Matrix Multiplication");

GRAS_DEFINE_TYPE(s_matrix,struct s_matrix {
  int lines;
  int rows;
  double *data GRAS_ANNOTE(size, lines*rows);
};)
typedef struct s_matrix matrix_t;

/* struct for recovering results */
GRAS_DEFINE_TYPE(s_result,struct s_result {
  int i;
  int j;
  double value;
});
typedef struct s_result result_t;

/* struct to send initial data to slave */
GRAS_DEFINE_TYPE(s_assignment,struct s_assignment {
  int linepos;
  int rowpos;
  xbt_host_t line[PROC_MATRIX_SIZE];
  xbt_host_t row[PROC_MATRIX_SIZE];
  double a;
  double b;
});
typedef struct s_assignment assignment_t;

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


/***  Function initilaze matrixs ***/

static void initmatrix(matrix_t *X){
  int i;

  for(i=0 ; i<(X->lines)*(X->rows); i++)
    X->data[i]=1.0;//*rand()/(RAND_MAX+1.0);
} /* end_of_initmatrixs */

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

/***  Function: Display Matrix ***/

static void display(matrix_t X){
	
  int i,j,t=0;

  printf("      ");
  for(j=0;j<X.rows;j++)
    printf("%.3d ",j);
  printf("\n");
  printf("    __");
  for(j=0;j<X.rows;j++)
    printf("____");
  printf("_\n");

  for(i=0;i<X.lines;i++){
    printf("%.3d | ",i);
    for(j=0;j<X.rows;j++)
      printf("%.3g ",X.data[t++]);
    printf("|\n");
  }
  printf("    --");
  for(j=0;j<X.rows;j++)
    printf("----");
  printf("-\n");

}/* end_of_display */

int master (int argc,char *argv[]) {

  xbt_ex_t e;

  int i,port,ask_result,step;

  matrix_t A,B,C;
  result_t result;

  gras_socket_t from;

  /*  Init the GRAS's infrastructure */
  gras_init(&argc, argv);

  xbt_host_t grid[SLAVE_COUNT]; /* The slaves */
  gras_socket_t socket[SLAVE_COUNT]; /* sockets for brodcast to slaves */

  /*  Initialize Matrixs */

  A.lines=A.rows=DATA_MATRIX_SIZE;
  B.lines=B.rows=DATA_MATRIX_SIZE;
  C.lines=C.rows=DATA_MATRIX_SIZE;
	
  A.data=xbt_malloc0(sizeof(double)*DATA_MATRIX_SIZE*DATA_MATRIX_SIZE);
  B.data=xbt_malloc0(sizeof(double)*DATA_MATRIX_SIZE*DATA_MATRIX_SIZE);
  C.data=xbt_malloc0(sizeof(double)*DATA_MATRIX_SIZE*DATA_MATRIX_SIZE);
	
  initmatrix(&A);
  initmatrix(&B);
	
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

  for( i=1;i< argc;i++){
    grid[i-1]=xbt_host_from_string(argv[i]);
    socket[i-1]=gras_socket_client(grid[i-1]->name,grid[i-1]->port);
      
    INFO2("Connected to %s:%d.",grid[i-1]->name,grid[i-1]->port);
  }

  int row=1, line=1;
  for(i=0 ; i<SLAVE_COUNT; i++){
    assignment_t assignment;
    int j;

    assignment.linepos=line;  // My line
    assignment.rowpos=row;  // My row


    /* Neiborhood */
    for (j=0; j<PROC_MATRIX_SIZE; j++) {
      assignment.row[j] = grid[ j*PROC_MATRIX_SIZE+(row-1) ] ;
      assignment.line[j] =  grid[ (line-1)*PROC_MATRIX_SIZE+j ] ;
    }

    row++;
    if (row > PROC_MATRIX_SIZE) {
      row=1;
      line++;
    }
		
    assignment.a=A.data[(line-1)*PROC_MATRIX_SIZE+(row-1)];
    assignment.b=B.data[(line-1)*PROC_MATRIX_SIZE+(row-1)];;
		
    gras_msg_send(socket[i],gras_msgtype_by_name("assignment"),&assignment);
    //    INFO3("Send assignment to %s : data A= %.3g & data B= %.3g",
    //	  gras_socket_peer_name(socket[i]),mydata.a,mydata.b);

  }
  // end assignment

  /******************************* multiplication ********************************/
  INFO0("XXXXXXXXXXXXXXXXXXXXXX begin Multiplication");
	
  for (step=1; step <= PROC_MATRIX_SIZE; step++){
    for (i=0; i< SLAVE_COUNT; i++){
      TRY {
	gras_msg_send(socket[i], gras_msgtype_by_name("step"), &step);
      } CATCH(e) {
	gras_socket_close(socket[i]);
	RETHROW0("Unable to send the msg : %s");
      }
    }
    INFO1("send to slave to begin a %d th step",step);

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
    C.data[(result.i-1)*DATA_MATRIX_SIZE+(result.j-1)]=result.value;
  }
  /*    end of gather   */
  INFO0 ("The Result of Multiplication is :");
  display(C);

  return 0;
} /* end_of_master */

/* **********************************************************************
 * slave code
 * **********************************************************************/

int slave(int argc,char *argv[]) {

  xbt_ex_t e; 

  int step,port,l,result_ack=0; 
  double bA,bB;

  int myline,myrow;
  double mydataA,mydataB;
  double bC=0;
  
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
  assignment_t assignment;
  INFO2("Launch %s (port=%d); wait for my enrole message",argv[0],port);
  TRY {
    gras_msg_wait(600,gras_msgtype_by_name("assignment"),&master,&assignment);
  } CATCH(e) {
    RETHROW0("Can't get my assignment from master : %s");
  }
  myline  = assignment.linepos;
  myrow   = assignment.rowpos;
  mydataA = assignment.a;
  mydataB = assignment.b;
  INFO4("Receive my pos (%d,%d) and assignment ( A=%.3g | B=%.3g )",
	myline,myrow,mydataA,mydataB);

  /* Get my neighborhood from the assignment message */
  int j=0;
  for (i=0,j=0 ; i<PROC_MATRIX_SIZE ; i++){
    if (strcmp(gras_os_myname(),assignment.line[i]->name)) {
      socket_line[j]=gras_socket_client(assignment.line[i]->name,
					assignment.line[i]->port);
      j++;
      //INFO3("Line neighbour %d: %s:%d",j,mydata.line[i]->name,mydata.line[i]->port);
    }
    xbt_host_free(assignment.line[i]);
  }
  for (i=0,j=0 ; i<PROC_MATRIX_SIZE ; i++){
    if (strcmp(gras_os_myname(),assignment.row[i]->name)) {
      socket_row[j]=gras_socket_client(assignment.row[i]->name,
				       assignment.row[i]->port);
      //INFO3("Row neighbour %d : %s:%d",j,mydata.row[i]->name,mydata.row[i]->port);
      j++;
    }
    xbt_host_free(assignment.row[i]);    
  }

  step=1;
  
  do {  //repeat until compute Cb
    step=PROC_MATRIX_SIZE+1;  // just intilization for loop
	
    TRY {
      gras_msg_wait(200,gras_msgtype_by_name("step"),NULL,&step);
    } CATCH(e) {
      RETHROW0("Can't get a Next Step message from master : %s");
    }
    INFO1("Receive a step message from master: step = %d ",step);

    if (step < PROC_MATRIX_SIZE ){
      /* a line brodcast */
      gras_os_sleep(3);  // IL FAUT EXPRIMER LE TEMPS D'ATTENTE EN FONCTION DE "SLAVE_COUNT"
      if(myline==step){
	INFO2("step(%d) = Myline(%d)",step,myline);
	for (l=1;l < PROC_MATRIX_SIZE ;l++){
	  gras_msg_send(socket_row[l-1], gras_msgtype_by_name("dataB"), &mydataB);
	  bB=mydataB;
	  INFO1("send my data B (%.3g) to my (vertical) neighbors",bB);  
	}
      }
      if(myline != step){ 
	INFO2("step(%d) <> Myline(%d)",step,myline);
	TRY {
	  gras_msg_wait(600,gras_msgtype_by_name("dataB"),&from,&bB);
	} CATCH(e) {
	  RETHROW0("Can't get a data message from line : %s");
	}
	INFO2("Receive data B (%.3g) from my neighbor: %s",bB,gras_socket_peer_name(from));
      }
      /* a row brodcast */
      if(myrow==step){
	for (l=1;l < PROC_MATRIX_SIZE ;l++){
	  gras_msg_send(socket_line[l-1],gras_msgtype_by_name("dataA"), &mydataA);
	  bA=mydataA;
	  INFO1("send my data A (%.3g) to my (horizontal) neighbors",bA);
	}
      }

      if(myrow != step){
	TRY {
	  gras_msg_wait(1200,gras_msgtype_by_name("dataA"), &from,&bA);
	} CATCH(e) {
	  RETHROW0("Can't get a data message from row : %s");
	}
	INFO2("Receive data A (%.3g) from my neighbor : %s ",bA,gras_socket_peer_name(from));
      }
      bC+=bA*bB;
      INFO1(">>>>>>>> My BC = %.3g",bC);

      /* send a ack msg to master */
	
      gras_msg_send(master,gras_msgtype_by_name("step_ack"),&step);
	
      INFO1("Send ack to master for to end %d th step",step);
    }
    if(step==PROC_MATRIX_SIZE-1) break;
	
  } while (step < PROC_MATRIX_SIZE);
  /*  wait Message from master to send the result */
 
  result.value=bC;
  result.i=myline;
  result.j=myrow;
 
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
  INFO3(">>>>>>>> Result: %.3f sent to %s:%d <<<<<<<<",
	bC,
	gras_socket_peer_name(master),gras_socket_peer_port(master));
  /*  Free the allocated resources, and shut GRAS down */
  gras_socket_close(master);
  gras_socket_close(from);
  gras_exit();
  INFO0("Done.");
  return 0;
} /* end_of_slave */
