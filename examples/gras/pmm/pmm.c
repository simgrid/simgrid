/* pmm - paralel matrix multiplication "double diffusion"                       */

/* Copyright (c) 2006- Ahmed Harbaoui. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"
#define MATRIX_SIZE 3

XBT_LOG_NEW_DEFAULT_CATEGORY(pmm,"Messages specific to this example");

GRAS_DEFINE_TYPE(s_matrix,struct s_matrix {
	int rows;
	int cols;
	double *data GRAS_ANNOTE(size, rows*cols);
};)
typedef struct s_matrix matrix_t;

/* struct for recovering results */
GRAS_DEFINE_TYPE(s_result,struct s_result {
	int i;
	int j;
	double value;
});
typedef struct s_result result_t;

/* struct to send initial data to sensor */
GRAS_DEFINE_TYPE(s_init_data,struct s_init_data {
	int myrow;
	int mycol;
	double a;
	double b;
});
typedef struct s_init_data init_data_t;

/* register messages which may be sent (common to client and server) */
static void register_messages(void) {
	gras_datadesc_type_t result_type;
	gras_datadesc_type_t init_data_type;
	result_type=gras_datadesc_by_symbol(s_result);
	init_data_type=gras_datadesc_by_symbol(s_init_data);
	
	gras_msgtype_declare("result", result_type);
	gras_msgtype_declare("init_data", init_data_type);

	gras_msgtype_declare("ask_result", gras_datadesc_by_name("int"));	
	gras_msgtype_declare("step", gras_datadesc_by_name("int"));
	gras_msgtype_declare("step_ack", gras_datadesc_by_name("int"));
	gras_msgtype_declare("data", gras_datadesc_by_name("int"));
}

/* Function prototypes */
int maestro (int argc,char *argv[]);
int sensor (int argc,char *argv[]);

/* **********************************************************************
 * Maestro code
 * **********************************************************************/

/* Global private data */
typedef struct {
  int nbr_col,nbr_row;
  int remaining_step;
  int remaining_ack;
} maestro_data_t;


static int maestro_cb_data_handler(gras_msg_cb_ctx_t ctx, void *payload) {

  xbt_ex_t e;
  /* 1. Get the payload into the msg variable */
  int msg=*(int*)payload_data;

  gras_socket_t expeditor = gras_msg_cb_ctx_from(ctx);

  /*code of callback */
   
  /* 8. Make sure we don't leak sockets */
  gras_socket_close(expeditor);
   
  /* 9. Tell GRAS that we consummed this message */
  return 1;
} /* end_of_maestro_cb_data_handler */

/***  Function initilaze matrixs ***/

static void initmatrix(matrix_t *X){
	int i;
	for(i=0 ; i<X.rows*X.cols; i++)
		X.data[i]=1.0;//1.0*rand()/(RAND_MAX+1.0);

} /* end_of_initmatrixs */

/***  Function Scatter Sequentiel ***/

static void scatter(){

}/* end_of_Scatter */

/***  Function: Scatter // ***/

static void scatter_parl(){

}/* end_of_Scatter // */

/***  Function: multiplication ***/

static void multiplication(){
	
  int step,i;
	
  for (step=1; step <= MATRIX_SIZE; step++){
    for (i=0; i< nbr_sensor; i++){
       TRY {
	       gras_msg_send(proc[(i/3)+1][(i%3)+1], gras_msgtype_by_name("step"), &step);  /* initialize Mycol, MyRow, mydataA,mydataB*/
  
	       myrow,mycol,mydataA,mydataB
       } CATCH(e) {
	gras_socket_close(proc[(i/3)+1][(i%3)+1]);
	RETHROW0("Unable to send the msg : %s");
       }
    }
    /* wait for computing and sensor messages exchange */
    TRY {
	    gras_msg_wait(600,gras_msgtype_by_name("init_data"),&from,&mydata);
    } CATCH(e) {
	    RETHROW0("I Can't get a init Data message from Maestro : %s");
    }
  }

}/* end_of_multiplication */

/***  Function: gather ***/

static void gather(){

	
}/* end_of_gather */

/***  Function: Display Matrix ***/

static void display(matrix_t X){
	
int i,j,t=0;

  printf("      ");
  for(j=0;j<X.cols;j++)
    printf("%.3d ",j);
    printf("\n");
    printf("    __");
    for(j=0;j<X.cols;j++)
	printf("____");
	printf("_\n");

	for(i=0;i<X.rows;i++){
	  printf("%.3d | ",i);
	  for(j=0;j<X.cols;j++)
	    printf("%.3g ",X.data[t++]);
	  printf("|\n");
	}
	printf("    --");
	for(j=0;j<X.cols;j++)
		printf("----");
	printf("-\n");

}/* end_of_display */

int maestro (int argc,char *argv[]) {

xbt_ex_t e;
int i,ask_result,step;
result_t result;
matrix_t A,B,C;

  gras_socket_t socket[MATRIX_SIZE*MATRIX_SIZE]; /* sockets for brodcast to other sensor */
	

  /*  Initialize Matrixs */

	A.rows=A.cols=MATRIX_SIZE;
	B.rows=B.cols=MATRIX_SIZE;
	C.rows=C.cols=MATRIX_SIZE;
	
	A.data=xbt_malloc0(sizeof(double)*MATRIX_SIZE*MATRIX_SIZE);
	B.data=xbt_malloc0(sizeof(double)*MATRIX_SIZE*MATRIX_SIZE);
	C.data=xbt_malloc0(sizeof(double)*MATRIX_SIZE*MATRIX_SIZE);
	
	initmatrix(&A);
	initmatrix(&B);
	
	/*  Init the GRAS's infrastructure */
	gras_init(&argc, argv);
	/*  Get arguments and create sockets */
	port=atoi(argv[1]);
	//scatter();multiplication();gather();
	//scatter_parl();
	/****************************** Init Data Send *********************************/
	int j=0;
	init_data_t mydata;
	for( i=2;i< argc;i+=3){
		
		TRY {
			socket[j]=gras_socket_client(argv[i],port);
			
		} CATCH(e) {
			RETHROW0("Unable to connect to the server: %s");
		}
		INFO2("Connected to %s:%d.",argv[i],port);
		
		mydata.myrow=argv[i+1];  // My rank of row
		mydata.mycol=argv[i+2];  // My rank of column
		mydata.a=A.data[(mydata.myrow-1)*MATRIX_SIZE+(mydata.mycol-1)];
		mydata.b=B.data[(mydata.myrow-1)*MATRIX_SIZE+(mydata.mycol-1)];;
		
		gras_msg_send(socket[j],gras_msgtype_by_name("init_data"),&mydata);
		j++;
	} // end init Data Send

	/******************************* multiplication ********************************/

	for (step=1; step <= MATRIX_SIZE; step++){
		for (i=0; i< nbr_sensor; i++){
		TRY {
		gras_msg_send(socket[i], gras_msgtype_by_name("step"), &step);  /* initialize Mycol, MyRow, mydataA,mydataB*/
  
		myrow,mycol,mydataA,mydataB
		    } CATCH(e) {
		gras_socket_close(socket[i]);
		RETHROW0("Unable to send the msg : %s");
		}
	}
	
	/* wait for computing and sensor messages exchange */
	for (i=0; i< nbr_sensor; i++){
		TRY {
		gras_msg_wait(600,gras_msgtype_by_name(""),&from,&mydata);
		} CATCH(e) {
		RETHROW0("I Can't get a init Data message from Maestro : %s");
		}
	}
	}
	/*********************************  gather ***************************************/
	
	int ask_result=0;
	for( i=1;i< argc;i++){
		gras_msg_send(socket[i],gras_msgtype_by_name("ask_result"),&ask_result);
	}
	/* wait for results */
	for( i=1;i< argc;i++){
		gras_msg_wait(600,gras_msgtype_by_name("result"),&from,&result);
		C.data[(result.i-1)*MATRIX_SIZE+(result.j-1)]=result.value;
	}
	/*    end of gather   */
	display(C);

return 0;
} /* end_of_maestro */

/* **********************************************************************
 * Sensor code
 * **********************************************************************/

int sensor(int argc,char *argv[]) {

  xbt_ex_t e; 

  static int bC=0;
  static int myrow,mycol;
  static double mydataA,mydataB;
  int bA,bB;
  int step,l,result=0;

  gras_socket_t from;  /* to recive from server for steps */

  gras_socket_t socket_row[2],socket_column[2]; /* sockets for brodcast to other sensor */

  /* Init the GRAS's infrastructure */

  gras_init(&argc, argv);

  /* Get arguments and create sockets */

  port=atoi(argv[1]);
  int i;
  for (i=1;i<MATRIX_SIZE;i++){
  socket_row[i]=gras_socket_client(argv[i+1],port);
  socket_column[i]=gras_socket_client(argv[i+MATRIX_SIZE],port);
  }
  INFO2("Launch %s (port=%d)",argv[0],port);

  /*  Create my master socket */
  sock = gras_socket_server(port);

  /*  Register the known messages */
  register_messages();

  /* Recover my initialized Data and My Position*/
  init_data_t mydata;

  TRY {
	  gras_msg_wait(600,gras_msgtype_by_name("init_data"),&from,&mydata);
  } CATCH(e) {
	RETHROW0("I Can't get a init Data message from Maestro : %s");
  }
  myrow=mydata.myrow;
  mycol=mydata.mycol;
  mydataA=mydata.a;
  mydataB=mydata.b;

  INFO4("Recover MY POSITION (%d,%d) and MY INIT DATA ( A=%.3g | B=%.3g )",
	myrow,mycol,mydataA,mydataB);


  do {  //repeat until compute Cb
	step=MATRIX_SIZE+1;  // juste intilization for loop

  TRY {
	gras_msg_wait(600,gras_msgtype_by_name("step"),&from,&step);
  } CATCH(e) {
	  RETHROW0("I Can't get a Next Step message from Maestro : %s");
  }

  /*  Wait for sensors startup */
  gras_os_sleep(1);

  if (step < MATRIX_SIZE){
	  /* a row brodcast */
	  if(myrow==step){
		  for (l=1;l < MATRIX_SIZE ;l++){
			  gras_msg_send(socket_row[l], gras_msgtype_by_name("data"), &mydataB);
			  bB=mydataB;
		  }
	  }
	  else
	  {
		  TRY {
			  gras_msg_wait(600,gras_msgtype_by_name("data"),
					&from,&bB);
		  } CATCH(e) {
			  RETHROW0("I Can't get a data message from row : %s");
		  }
	  }
	  /* a column brodcast */	
	  if(mycol==step){
		  for (l=1;l < MATRIX_SIZE ;l++){
			  gras_msg_send(socket_column[l],gras_msgtype_by_name("data"), &mydataA);
			  bA=mydataA;
		  }
	  }
	  else
	  {
		  TRY {
			  gras_msg_wait(600,gras_msgtype_by_name("data"),
					&from,&bA);
		  } CATCH(e) {
			  RETHROW0("I Can't get a data message from column : %s");
		  }
	  }
	  bC+=bA*bB;
	  }
	  /* send a ack msg to Maestro */
	
	  gras_msg_send(from,gras_msgtype_by_name("step_ack"),&step);
	
	  INFO1("Send ack to maestro for to end %d th step",step);
	
	  if(step==MATRIX_SIZE-1) break;

  } while (step < MATRIX_SIZE);
    /*  wait Message from maestro to send the result */
	    /*after finished the bC computing */
	  TRY {
		  gras_msg_wait(600,gras_msgtype_by_name("result"),
				&from,&result);
	  } CATCH(e) {
		  RETHROW0("I Can't get a data message from row : %s");
	  }
	  /* 5. send Result to the Maestro */
	  TRY {
		  gras_msg_send(from, gras_msgtype_by_name("result"),&bC);
	  } CATCH(e) {
		  gras_socket_close(from);
		  RETHROW0("Failed to send PING to server: %s");
	  }
	  INFO3(">>>>>>>> Result: %d sent to %s:%d <<<<<<<<",
		bC,
		gras_socket_peer_name(from),gras_socket_peer_port(from));
    /*  Free the allocated resources, and shut GRAS down */
	  gras_socket_close(from);
	  gras_exit();
	  INFO0("Done.");
	  return 0;
} /* end_of_sensor */