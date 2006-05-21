/* pmm - paralel matrix multiplication "double diffusion"                       */

/* Copyright (c) 2006- Ahmed Harbaoui. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"
#define MATRIX_SIZE 3
#define SENSOR_NBR 9

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
	
	gras_msgtype_declare("result", result_type);  // recieve a final result from sensor
	gras_msgtype_declare("init_data", init_data_type);  // send from maestro to sensor to initialize data bA,bB

	gras_msgtype_declare("ask_result", gras_datadesc_by_name("int")); // send from maestro to sensor to ask a final result	
	gras_msgtype_declare("step", gras_datadesc_by_name("int"));// send from maestro to sensor to indicate the begining of step 
	gras_msgtype_declare("step_ack", gras_datadesc_by_name("int"));//send from sensor to maestro to indicate the end of the current step
	gras_msgtype_declare("dataA", gras_datadesc_by_name("double"));// send data between sensor
	gras_msgtype_declare("dataB", gras_datadesc_by_name("double"));// send data between sensor
}

/* Function prototypes */
int sensor (int argc,char *argv[]);
int maestro (int argc,char *argv[]);


/* **********************************************************************
 * Maestro code
 * **********************************************************************/

/* Global private data */
typedef struct {
  int nbr_col,nbr_row;
  int remaining_step;
  int remaining_ack;
} maestro_data_t;


/***  Function initilaze matrixs ***/

static void initmatrix(matrix_t *X){
int i;

for(i=0 ; i<(X->rows)*(X->cols); i++)
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

int i,port,ask_result,step;

matrix_t A,B,C;
result_t result;

gras_socket_t from;

	/*  Init the GRAS's infrastructure */
	gras_init(&argc, argv);

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
	
	/*  Get arguments and create sockets */
	port=atoi(argv[1]);
	//scatter();
	//scatter_parl();
	//multiplication();
	//gather();
	//display(A);
	/****************************** Init Data Send *********************************/
	int step_ack,j=0;
	init_data_t mydata;
	gras_os_sleep(60);                                                // MODIFIER LES TEMPS D'ATTENTE 60 c trop normalement
	for( i=2;i< argc;i+=3){
		TRY {
		socket[j]=gras_socket_client(argv[i],port);
		} CATCH(e) {
			RETHROW0("Unable to connect to the server: %s");
		}
		INFO2("Connected to %s:%d.",argv[i],port);
		
		mydata.myrow=atoi(argv[i+1]);  // My row
		mydata.mycol=atoi(argv[i+2]);  // My column
		
		mydata.a=A.data[(mydata.myrow-1)*MATRIX_SIZE+(mydata.mycol-1)];
		mydata.b=B.data[(mydata.myrow-1)*MATRIX_SIZE+(mydata.mycol-1)];;
		
		gras_msg_send(socket[j],gras_msgtype_by_name("init_data"),&mydata);
		INFO3("Send Init Data to %s : data A= %.3g & data B= %.3g",gras_socket_peer_name(socket[j]),mydata.a,mydata.b);
		j++;
	} // end init Data Send

	/******************************* multiplication ********************************/
	INFO0("begin Multiplication");
	
	for (step=1; step <= MATRIX_SIZE; step++){
		gras_os_sleep(50);
		for (i=0; i< SENSOR_NBR; i++){
		TRY {
		gras_msg_send(socket[i], gras_msgtype_by_name("step"), &step);  /* initialize Mycol, MyRow, mydataA,mydataB*/
		} CATCH(e) {
		gras_socket_close(socket[i]);
		RETHROW0("Unable to send the msg : %s");
		}
	}
	INFO1("send to sensor to begin a %d th step",step);
	/* wait for computing and sensor messages exchange */
	i=0;
	
	while  ( i< SENSOR_NBR){
		TRY {
		gras_msg_wait(1300,gras_msgtype_by_name("step_ack"),&from,&step_ack);
		} CATCH(e) {
		RETHROW0("I Can't get a Ack step message from sensor : %s");
		}
		i++;
		INFO1("Recive Ack step ack from %s",gras_socket_peer_name(from));
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
		C.data[(result.i-1)*MATRIX_SIZE+(result.j-1)]=result.value;
	}
	/*    end of gather   */
	INFO0 ("The Result of Multiplication is :");
	display(C);

return 0;
} /* end_of_maestro */

/* **********************************************************************
 * Sensor code
 * **********************************************************************/

int sensor(int argc,char *argv[]) {

  xbt_ex_t e; 

  int step,port,l,result_ack=0; 
  double bA,bB;

  int myrow,mycol;
  double mydataA,mydataB;
  double bC=0;
  
  static end_step;

  result_t result;
 
  gras_socket_t from,sock;  /* to recive from server for steps */

  gras_socket_t socket_row[MATRIX_SIZE-1],socket_column[MATRIX_SIZE-1]; /* sockets for brodcast to other sensor */

  /* Init the GRAS's infrastructure */

  gras_init(&argc, argv);

  /* Get arguments and create sockets */

  port=atoi(argv[1]);
  
  /*  Create my master socket */
  sock = gras_socket_server(port);
  INFO2("Launch %s (port=%d)",argv[0],port);
  gras_os_sleep(1); //wait to start all sensor 

  int i;
  for (i=1;i<MATRIX_SIZE;i++){
  socket_row[i-1]=gras_socket_client(argv[i+1],port);
  socket_column[i-1]=gras_socket_client(argv[i+MATRIX_SIZE],port);
  }

  /*  Register the known messages */
  register_messages();

  /* Recover my initialized Data and My Position*/
  init_data_t mydata;
  INFO0("wait for init Data");
  TRY {
	  gras_msg_wait(600,gras_msgtype_by_name("init_data"),&from,&mydata);
  } CATCH(e) {
	RETHROW0("I Can't get a init Data message from Maestro : %s");
  }
  myrow=mydata.myrow;
  mycol=mydata.mycol;
  mydataA=mydata.a;
  mydataB=mydata.b;
  INFO4("Recive MY POSITION (%d,%d) and MY INIT DATA ( A=%.3g | B=%.3g )",
	myrow,mycol,mydataA,mydataB);
  step=1;
  
  do {  //repeat until compute Cb
	step=MATRIX_SIZE+1;  // just intilization for loop
	
  TRY {
	gras_msg_wait(60,gras_msgtype_by_name("step"),&from,&step);
 } CATCH(e) {
	  RETHROW0("I Can't get a Next Step message from Maestro : %s");
 }
  INFO1("Recive a step message from maestro: step = %d ",step);

  if (step < MATRIX_SIZE ){
	  /* a row brodcast */
	  gras_os_sleep(3);  // IL FAUT EXPRIMER LE TEMPS D'ATTENTE EN FONCTION DE "SENSOR_NBR"
	  if(myrow==step){
		INFO2("step(%d) = Myrow(%d)",step,myrow);
		for (l=1;l < MATRIX_SIZE ;l++){
		  gras_msg_send(socket_column[l-1], gras_msgtype_by_name("dataB"), &mydataB);
		  bB=mydataB;
		INFO1("send my data B (%.3g) to my (vertical) neighbors",bB);  
		}
	}
	if(myrow != step){ 
		INFO2("step(%d) <> Myrow(%d)",step,myrow);
		TRY {
		  gras_msg_wait(600,gras_msgtype_by_name("dataB"),
				&from,&bB);
		} CATCH(e) {
		  RETHROW0("I Can't get a data message from row : %s");
		}
		INFO2("Recive data B (%.3g) from my neighbor: %s",bB,gras_socket_peer_name(from));
	  }
	  /* a column brodcast */
	  if(mycol==step){
		for (l=1;l < MATRIX_SIZE ;l++){
			gras_msg_send(socket_row[l-1],gras_msgtype_by_name("dataA"), &mydataA);
			bA=mydataA;
			INFO1("send my data A (%.3g) to my (horizontal) neighbors",bA);
		  }
	  }

	if(mycol != step){
		TRY {
		   gras_msg_wait(1200,gras_msgtype_by_name("dataA"),
				&from,&bA);
		} CATCH(e) {
		  RETHROW0("I Can't get a data message from column : %s");
		}
		INFO2("Recive data A (%.3g) from my neighbor : %s ",bA,gras_socket_peer_name(from));
	}
	  bC+=bA*bB;
	  INFO1(">>>>>>>> My BC = %.3g",bC);

	  /* send a ack msg to Maestro */
	
	  gras_msg_send(from,gras_msgtype_by_name("step_ack"),&step);
	
	  INFO1("Send ack to maestro for to end %d th step",step);
	}
	  if(step==MATRIX_SIZE-1) break;
	
  } while (step < MATRIX_SIZE);
    /*  wait Message from maestro to send the result */
 
	result.value=bC;
	result.i=myrow;
	result.j=mycol;
 
	  TRY {
		  gras_msg_wait(600,gras_msgtype_by_name("ask_result"),
				&from,&result_ack);
	  } CATCH(e) {
		  RETHROW0("I Can't get a data message from row : %s");
	  }
	  /* send Result to Maestro */
	  TRY {
		  gras_msg_send(from, gras_msgtype_by_name("result"),&result);
	  } CATCH(e) {
		 // gras_socket_close(from);
		  RETHROW0("Failed to send PING to server: %s");
	  }
	  INFO3(">>>>>>>> Result: %.3f sent to %s:%d <<<<<<<<",
		bC,
		gras_socket_peer_name(from),gras_socket_peer_port(from));
    /*  Free the allocated resources, and shut GRAS down */
	  gras_socket_close(from);
	  gras_exit();
	  INFO0("Done.");
	  return 0;
} /* end_of_sensor */