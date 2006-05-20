/* $Id$ */

/* Simple Generator                                                         */
/* generates SimGrid platforms being either lines, rings, mesh, tore or     */
/*  hypercubes.                                                             */

/* Copyright (C) 2006 Flavien Vernier (original code)                       */
/* Copyright (C) 2006 Martin Quinson  (cleanups & maintainance)             */
/* All right reserved.                                                      */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include<stdio.h>
#include<math.h>
#include <stdlib.h>

int MFLOPS=1000;
int MBYTES=100;
double LAT_S=0.00015;

int size=0;
int x=0,y=0;
int dimension=0;

int rand_mflops_min=0, rand_mflops_max=0;
int rand_mbytes_min=0, rand_mbytes_max=0;
double rand_lat_s_min=0.0, rand_lat_s_max=0.0;

void randParam(){
  if(rand_mflops_min!=rand_mflops_max){
    MFLOPS=rand_mflops_min+((double)(rand_mflops_max-rand_mflops_min)*((double)rand()/RAND_MAX));
  }
  if(rand_mbytes_min!=rand_mbytes_max){
    MBYTES=rand_mbytes_min+((double)(rand_mbytes_max-rand_mbytes_min)*((double)rand()/RAND_MAX));
  }
  if(rand_lat_s_min!=rand_lat_s_max){
    LAT_S=rand_lat_s_min+((rand_lat_s_max-rand_lat_s_min)*((double)rand()/RAND_MAX));
  }
}

void generateLine(){
  int i;
  fprintf(stdout,"<?xml version='1.0'?>\n<!DOCTYPE platform_description SYSTEM \"surfxml.dtd\">\n<platform_description>\n");
  
  
  for(i=0;i<size;i++){
    fprintf(stdout,"<cpu name=\"node%i\" power=\"%i\"/>\n",i,MFLOPS);
    randParam();
  }
  
  for(i=0;i<2*(size-1);i++){
    fprintf(stdout,"<network_link name=\"link%i\" bandwidth=\"%i\" latency=\"%f\"/>\n",i,MBYTES,LAT_S);
    randParam();
  }

  for(i=0;i<size-1;i++){
    fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",i,(i+1),2*i);
    fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",(i+1),i,2*i+1);
  }
  fprintf(stdout,"</platform_description>\n");

}

void generateRing(){
  int i;
  fprintf(stdout,"<?xml version='1.0'?>\n<!DOCTYPE platform_description SYSTEM \"surfxml.dtd\">\n<platform_description>\n");
  
  
  for(i=0;i<size;i++){
    fprintf(stdout,"<cpu name=\"node%i\" power=\"%i\"/>\n",i,MFLOPS);
    randParam();
  }
  
  for(i=0;i<2*size;i++){
    fprintf(stdout,"<network_link name=\"link%i\" bandwidth=\"%i\" latency=\"%f\"/>\n",i,MBYTES,LAT_S);
    randParam();
  }

  for(i=0;i<size;i++){
    fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",i,(i+1)%size,2*i);
    fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",(i+1)%size,i,2*i+1);
  }
  fprintf(stdout,"</platform_description>\n");
}

void generateMesh(){
  int i,j,l;
  fprintf(stdout,"<?xml version='1.0'?>\n<!DOCTYPE platform_description SYSTEM \"surfxml.dtd\">\n<platform_description>\n");
  
  for(i=0;i<x*y;i++){
    fprintf(stdout,"<cpu name=\"node%i\" power=\"%i\"/>\n",i,MFLOPS);
    randParam();
  }
  
  for(i=0;i<2*((x-1)*y+(y-1)*x);i++){
    fprintf(stdout,"<network_link name=\"link%i\" bandwidth=\"%i\" latency=\"%f\"/>\n",i,MBYTES,LAT_S);
    randParam();
  }

  l=0;
  for(j=0;j<y;j++){
    for(i=0;i<x;i++){
      if((i+1)%x!=0){
	fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",j*x+i,j*x+(i+1),l++);
	fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",j*x+(i+1),j*x+i,l++);
      }
      if(j<y-1){
	fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",j*x+i,j*x+(i+x),l++);
	fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",j*x+(i+x),j*x+i,l++);
      }
    }
  }
  fprintf(stdout,"</platform_description>\n");
}

void generateTore(){
  int i,j,l;
  fprintf(stdout,"<?xml version='1.0'?>\n<!DOCTYPE platform_description SYSTEM \"surfxml.dtd\">\n<platform_description>\n");
  
  for(i=0;i<x*y;i++){
    fprintf(stdout,"<cpu name=\"node%i\" power=\"%i\"/>\n",i,MFLOPS);
    randParam();
  }
  
  for(i=0;i<4*x*y;i++){
    fprintf(stdout,"<network_link name=\"link%i\" bandwidth=\"%i\" latency=\"%f\"/>\n",i,MBYTES,LAT_S);
    randParam();
  }

  l=0;
  for(j=0;j<y;j++){
    for(i=0;i<x;i++){
      if((i+1)%x!=0){
	fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",j*x+i,j*x+(i+1),l++);
	fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",j*x+(i+1),j*x+i,l++);
      }else{
	fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",j*x+i,j*x+i-x+1,l++);
	fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",j*x+i-x+1,j*x+i,l++);
      }
      if(j<y-1){
	fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",j*x+i,j*x+(i+x),l++);
	fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",j*x+(i+x),j*x+i,l++);
      }else{
	fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",j*x+i,i,l++);
	fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",i,j*x+i,l++);
      }
    }
  }
  fprintf(stdout,"</platform_description>\n");
}


void generateHypercube(){
  int i,d,l;

  fprintf(stdout,"<?xml version='1.0'?>\n<!DOCTYPE platform_description SYSTEM \"surfxml.dtd\">\n<platform_description>\n");
  
  for(i=0;i<(int)pow(2.0,(double)dimension);i++){
    fprintf(stdout,"<cpu name=\"node%i\" power=\"%i\"/>\n",i,MFLOPS);
    randParam();
  }

  for(i=0;i<dimension*(int)pow(2.0,(double)dimension);i++){
    fprintf(stdout,"<network_link name=\"link%i\" bandwidth=\"%i\" latency=\"%f\"/>\n",i,MBYTES,LAT_S);
    randParam();
  }

  fprintf(stdout,"ROUTES\n");
  l=0;
  for(i=0;i<(int)pow(2.0,(double)dimension);i++){
    for(d=1;d<(int)pow(2.0,(double)dimension);d=2*d)
      fprintf(stdout,"<route src=\"node%i\" dst=\"node%i\"> <route_element name=\"link%i\"/></route>\n",i,i^d,l++);
  }
  fprintf(stdout,"</platform_description>\n");
}

void usage(int argc, char** argv){
  fprintf(stderr,"%s topo [options] > platform_file.xml\n",argv[0]);
  fprintf(stderr,"\ntopo:\n");
  fprintf(stderr,"\t LINE n   \t generates a line of n nodes.\n");
  fprintf(stderr,"\t RING n   \t generates a ring of n nodes.\n");
  fprintf(stderr,"\t MESH x y \t generates a mesh of x line(s) y column(s).\n");
  fprintf(stderr,"\t TORE x y \t generates a tore of x line(s) y column(s).\n");
  fprintf(stderr,"\t HYPE d   \t generates a hypercubee of dimension d\n");
  fprintf(stderr,"\noptions:\n");
  fprintf(stderr,"\t --mflops n \t defines the power of each node (default: 1000), n must be integer\n");
  fprintf(stderr,"\t --mbytes n \t defines the bandwidth of each edge (default: 100), n must be integer\n");
  fprintf(stderr,"\t --lat_s  n \t defines the latency of each edge (default: 0.00015), n can be float\n");
  fprintf(stderr,"\t --rand_mflops min max \t defines a random power between min and max for each node\n");
  fprintf(stderr,"\t --rand_mbytes min max \t defines a random bandwidth between min and max for each edge\n");
  fprintf(stderr,"\t --rand_lat_s  min max \t defines a random latency between min and max for each edge\n");
  fprintf(stderr,"\t --help\n");
}


int main(int argc, char** argv){
  int i=1;

  void (*generate)(); 
  generate=NULL;

  srand(time(NULL));

  while(i<argc){
    if((strcmp(argv[i],"LINE")==0)&&(i+1<argc)){
      generate=generateLine;
      size=atoi(argv[i+1]);
      i+=2;
    }else if((strcmp(argv[i],"RING")==0)&&(i+1<argc)){
      generate=generateRing;
      size=atoi(argv[i+1]);
      i+=2;
    }else if((strcmp(argv[i],"MESH")==0)&&(i+2<argc)){
      generate=generateMesh;
      x=atoi(argv[i+1]);
      y=atoi(argv[i+2]);
      i+=3;
    }else if((strcmp(argv[i],"TORE")==0)&&(i+2<argc)){
      generate=generateTore;
      x=atoi(argv[i+1]);
      y=atoi(argv[i+2]);
      i+=3;
    }else if((strcmp(argv[i],"HYPE")==0)&&(i+1<argc)){
      generate=generateHypercube;
      dimension=atoi(argv[i+1]);
      i+=2;
    }else if((strcmp(argv[i],"--mflops")==0)&&(i+1<argc)){
      MFLOPS=atoi(argv[i+1]);
      i+=2;
    }else if((strcmp(argv[i],"--mbytes")==0)&&(i+1<argc)){
      MBYTES=atoi(argv[i+1]);
      i+=2;
    }else if((strcmp(argv[i],"--lat_s")==0)&&(i+1<argc)){
      LAT_S=atof(argv[i+1]);
      i+=2;
    }else if((strcmp(argv[i],"--rand_mflops")==0)&&(i+2<argc)){
      rand_mflops_min=atoi(argv[i+1]);
      rand_mflops_max=atoi(argv[i+2]);
      randParam();
      i+=3;
    }else if((strcmp(argv[i],"--rand_mbytes")==0)&&(i+2<argc)){
      rand_mbytes_min=atoi(argv[i+1]);
      rand_mbytes_max=atoi(argv[i+2]);
      randParam();
      i+=3;
    }else if((strcmp(argv[i],"--rand_lat_s")==0)&&(i+2<argc)){
      rand_lat_s_min=atof(argv[i+1]);
      rand_lat_s_max=atof(argv[i+2]);
      randParam();
      i+=3;
    }else{
      usage(argc,argv);
      return 1;
    }
  }
  if(generate!=NULL){
    (*generate)();
    return 0;
  }else{
      usage(argc,argv);
      return 1;    
  }
  return 0;
}

