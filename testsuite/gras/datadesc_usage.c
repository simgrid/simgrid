/* $Id$ */

/* datadesc: test of data description (using file transport).               */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <gras.h>

#include "../DataDesc/datadesc_interface.h"
GRAS_LOG_NEW_DEFAULT_CATEGORY(test);

#define READ  0
#define WRITE 1
#define RW    2

gras_error_t
write_read(gras_datadesc_type_t *type,void *src, void *dst, 
	   gras_socket_t *sock, int direction);

gras_error_t
write_read(gras_datadesc_type_t *type,void *src, void *dst, 
	   gras_socket_t *sock, int direction) {
  gras_error_t errcode;
   
  /* write */
  if (direction == RW) 
    TRY(gras_socket_client_from_file("datadesc_usage.out",&sock));
  if (direction == WRITE || direction == RW)
    TRY(gras_datadesc_send(sock, type, src));
  if (direction == RW) 
    gras_socket_close(&sock);
   
  /* read */
  if (direction == RW) 
    TRY(gras_socket_server_from_file("datadesc_usage.out",&sock));

  if (direction == READ || direction == RW)
    TRY(gras_datadesc_recv(sock, type, gras_arch_selfid(), dst));

  if (direction == RW) 
    gras_socket_close(&sock);
  
  return no_error;
}

gras_error_t test_int(gras_socket_t *sock, int direction);
gras_error_t test_float(gras_socket_t *sock, int direction);
gras_error_t test_array(gras_socket_t *sock, int direction);
gras_error_t test_intref(gras_socket_t *sock, int direction);
gras_error_t test_string(gras_socket_t *sock, int direction);

gras_error_t test_homostruct(gras_socket_t *sock, int direction);
gras_error_t test_hetestruct(gras_socket_t *sock, int direction);
gras_error_t test_nestedstruct(gras_socket_t *sock, int direction);
gras_error_t test_chain_list(gras_socket_t *sock, int direction);
gras_error_t test_graph(gras_socket_t *sock, int direction);

gras_error_t test_int(gras_socket_t *sock, int direction) {
  gras_error_t errcode;
  int i=5,j;
  
  INFO0("==== Test on integer ====");
  TRY(write_read(gras_datadesc_by_name("int"), &i,&j, sock,direction));
  if (direction == READ || direction == RW) {
    gras_assert(i == j);
  }
  return no_error;
}
gras_error_t test_float(gras_socket_t *sock, int direction) {
  gras_error_t errcode;
  float i=5.0,j;
  
  INFO0("==== Test on float ====");
  TRY(write_read(gras_datadesc_by_name("float"), &i,&j, sock,direction));
  if (direction == READ || direction == RW) {
    gras_assert(i == j);
  }
  return no_error;
}

#define SIZE 5
typedef int array[SIZE];
gras_error_t test_array(gras_socket_t *sock, int direction) {
  gras_error_t errcode;
  gras_datadesc_type_t *my_type;
  
  array i,j;
  int cpt;

  INFO0("==== Test on fixed array ====");
  for (cpt=0; cpt<SIZE; cpt++) {
    i[cpt] = rand();
  }

  TRY(gras_datadesc_declare_array_fixed("fixed int array", 
					gras_datadesc_by_name("int"),
					SIZE, &my_type));

  TRY(write_read(my_type, &i,&j, sock,direction));
  if (direction == READ || direction == RW) {
    for (cpt=0; cpt<SIZE; cpt++)
      gras_assert4(i[cpt] == j[cpt],"i[%d]=%d  !=  j[%d]=%d",
		   cpt,i[cpt],cpt,j[cpt]);
  }
  return no_error;
}
gras_error_t test_intref(gras_socket_t *sock, int direction) {
  gras_error_t errcode;
  gras_datadesc_type_t *my_type;
  int *i,*j;
  
  if (! (i=malloc(sizeof(int))) )
    RAISE_MALLOC;
  *i=12345;

  INFO1("==== Test on a reference to an integer (%p) ====",i);

  TRY(gras_datadesc_declare_ref("int*",gras_datadesc_by_name("int"),&my_type));

  TRY(write_read(my_type, &i,&j, sock,direction));
  if (direction == READ || direction == RW) {
    gras_assert(*i == *j);
    free(j);
  }
  free(i);
  return no_error;
}

/***
 *** string (dynamic array)
 ***/ 
gras_error_t test_string(gras_socket_t *sock, int direction) {
  gras_error_t errcode;
  char *i=strdup("Some data"), *j=NULL;
  int cpt;
  
  INFO0("==== Test on string (ref to dynamic array) ====");
  TRY(write_read(gras_datadesc_by_name("string"), &i,&j,
		 sock,direction));
  if (direction == READ || direction == RW) {
    for (cpt=0; cpt<strlen(i); cpt++) {
      gras_assert4(i[cpt] == j[cpt],"i[%d]=%c  !=  j[%d]=%c",
		   cpt,i[cpt],cpt,j[cpt]);
    } 
    free(j);
  }
  free(i);
  return no_error;
}


/***
 *** homogeneous struct
 ***/ 
typedef struct {
  int a,b,c,d;
} homostruct;
gras_error_t test_homostruct(gras_socket_t *sock, int direction) {
  gras_error_t errcode;
  gras_datadesc_type_t *my_type;
  homostruct *i, *j; 

  INFO0("==== Test on homogeneous structure ====");
  /* create descriptor */
  TRY(gras_datadesc_declare_struct("homostruct",&my_type));
  TRY(gras_datadesc_declare_struct_append(my_type,"a",
					  gras_datadesc_by_name("signed int")));
  TRY(gras_datadesc_declare_struct_append(my_type,"b",
					  gras_datadesc_by_name("int")));
  TRY(gras_datadesc_declare_struct_append(my_type,"c",
					  gras_datadesc_by_name("int")));
  TRY(gras_datadesc_declare_struct_append(my_type,"d",
					  gras_datadesc_by_name("int")));
  TRY(gras_datadesc_declare_ref("homostruct*",
				gras_datadesc_by_name("homostruct"),
				&my_type));

  /* init a value, exchange it and check its validity*/
  if (! (i=malloc(sizeof(homostruct))) )
    RAISE_MALLOC;
  i->a = rand();  i->b = rand();
  i->c = rand();  i->d = rand();

  TRY(write_read(my_type, &i,&j, sock,direction));
  if (direction == READ || direction == RW) {
    gras_assert(i->a == j->a);
    gras_assert(i->b == j->b);
    gras_assert(i->c == j->c);
    gras_assert(i->d == j->d);
    free(j);
  }
  free(i);
  return no_error;
}

/***
 *** heterogeneous struct
 ***/ 
typedef struct {
  unsigned char c1;
  unsigned long int l1;
  unsigned char c2;
  unsigned long int l2;
} hetestruct;
gras_error_t test_hetestruct(gras_socket_t *sock, int direction) {
  gras_error_t errcode;
  gras_datadesc_type_t *my_type;
  hetestruct *i, *j; 

  INFO0("==== Test on heterogeneous structure ====");
  /* create descriptor */
  TRY(gras_datadesc_declare_struct("hetestruct",&my_type));
  TRY(gras_datadesc_declare_struct_append(my_type,"c1",
					  gras_datadesc_by_name("unsigned char")));
  TRY(gras_datadesc_declare_struct_append(my_type,"l1",
					  gras_datadesc_by_name("unsigned long int")));
  TRY(gras_datadesc_declare_struct_append(my_type,"c2",
					  gras_datadesc_by_name("unsigned char")));
  TRY(gras_datadesc_declare_struct_append(my_type,"l2",
					  gras_datadesc_by_name("unsigned long int")));
  TRY(gras_datadesc_declare_ref("hetestruct*",
				gras_datadesc_by_name("hetestruct"),
				&my_type));

  /* init a value, exchange it and check its validity*/
  if (! (i=malloc(sizeof(hetestruct))) )
    RAISE_MALLOC;
  i->c1 = 's'; i->l1 = 123455;
  i->c2 = 'e'; i->l2 = 774531;

  TRY(write_read(my_type, &i,&j, sock,direction));
  if (direction == READ || direction == RW) {
    gras_assert(i->c1 == j->c1);
    gras_assert(i->c2 == j->c2);
    gras_assert(i->l1 == j->l1);
    gras_assert(i->l2 == j->l2);
    free(j);
  }
  free(i);
  return no_error;
}

/***
 *** nested struct
 ***/ 
typedef struct {
  hetestruct hete;
  homostruct homo;
} nestedstruct;
gras_error_t test_nestedstruct(gras_socket_t *sock, int direction) {
  gras_error_t errcode;
  gras_datadesc_type_t *my_type;
  nestedstruct *i, *j; 

  INFO0("==== Test on nested structures ====");
  /* create descriptor */
  TRY(gras_datadesc_declare_struct("nestedstruct",&my_type));

  TRY(gras_datadesc_declare_struct_append(my_type,"hete",
					  gras_datadesc_by_name("hetestruct")));
  TRY(gras_datadesc_declare_struct_append(my_type,"homo",
					  gras_datadesc_by_name("homostruct")));
  TRY(gras_datadesc_declare_ref("nestedstruct*",
				gras_datadesc_by_name("nestedstruct"),
				&my_type));

  /* init a value, exchange it and check its validity*/
  if (! (i=malloc(sizeof(nestedstruct))) )
    RAISE_MALLOC;
  i->homo.a = rand();  i->homo.b = rand();
  i->homo.c = rand();  i->homo.d = rand();
  i->hete.c1 = 's'; i->hete.l1 = 123455;
  i->hete.c2 = 'e'; i->hete.l2 = 774531;

  TRY(write_read(my_type, &i,&j, sock,direction));
  if (direction == READ || direction == RW) {
    gras_assert(i->homo.a == j->homo.a);
    gras_assert(i->homo.b == j->homo.b);
    gras_assert(i->homo.c == j->homo.c);
    gras_assert(i->homo.d == j->homo.d);
    gras_assert(i->hete.c1 == j->hete.c1);
    gras_assert(i->hete.c2 == j->hete.c2);
    gras_assert(i->hete.l1 == j->hete.l1);
    gras_assert(i->hete.l2 == j->hete.l2);
    free(j);
  }
  free(i);
  return no_error;
}

/***
 *** chained list
 ***/ 
typedef struct s_chained_list chained_list_t;
struct s_chained_list {
  int          v;
  chained_list_t *l;
};
gras_error_t declare_chained_list_type(void);
chained_list_t *cons(int v, chained_list_t *l);
void list_free(chained_list_t *l);
int list_eq(chained_list_t*i,chained_list_t*j);

gras_error_t declare_chained_list_type(void) {
  gras_error_t errcode;
  gras_datadesc_type_t *my_type,*ref_my_type;

  TRY(gras_datadesc_declare_struct("chained_list_t",&my_type));
  TRY(gras_datadesc_declare_ref("chained_list_t*",my_type,&ref_my_type));

  TRY(gras_datadesc_declare_struct_append(my_type,"v",
					  gras_datadesc_by_name("int")));
  TRY(gras_datadesc_declare_struct_append(my_type,"l",ref_my_type));

  return no_error;
}

chained_list_t * cons(int v, chained_list_t *l) {
  chained_list_t *nl = malloc(sizeof (chained_list_t));
  
  nl->v = v;
  nl->l = l;
  
  return nl;
}
void list_free(chained_list_t*l) {
  if (l) {
    list_free(l->l);
    free(l);
  }
}
int list_eq(chained_list_t*i,chained_list_t*j) {
  if (!i || !j) return i == j;
  if (i->v != j->v)
    return 0;
  return list_eq(i->l, j->l); 
}
gras_error_t test_chain_list(gras_socket_t *sock, int direction) {
  gras_error_t errcode;
  chained_list_t *i, *j; 

  INFO0("==== Test on chained list ====");

  /* init a value, exchange it and check its validity*/
  i = cons( rand(), cons( rand() , cons( rand(), NULL)));
  j = NULL;

  TRY(write_read(gras_datadesc_by_name("chained_list_t*"),
		 &i,&j,
		 sock,direction));
  if (direction == READ || direction == RW) {
    gras_assert(list_eq(i,j));    
    list_free(j);
  }

  list_free(i);
  return no_error;
}
/***
 *** graph
 ***/
gras_error_t test_graph(gras_socket_t *sock, int direction) {
  gras_error_t errcode;
  chained_list_t *i, *j; 

  INFO0("==== Test on graph (cyclique chained list) ====");
  /* init a value, exchange it and check its validity*/
  i = cons( rand(), cons( rand() , cons( rand(), NULL)));
  i->l->l->l = i;
  j = NULL;

  TRY(write_read(gras_datadesc_by_name("chained_list_t*"),
		 &i,&j, sock,direction));
  if (direction == READ || direction == RW) {
    
    DEBUG1("i=%p"         ,i);
    DEBUG1("i->l=%p"      ,i->l);
    DEBUG1("i->l->l=%p"   ,i->l->l);
    DEBUG1("i->l->l->l=%p",i->l->l->l);
    DEBUG1("j=%p"         ,j);
    DEBUG1("j->l=%p"      ,j->l);
    DEBUG1("j->l->l=%p"   ,j->l->l);
    DEBUG1("j->l->l->l=%p",j->l->l->l);
    gras_assert4(j->l->l->l == j,
		 "Received list is not cyclic. j=%p != j->l->l->l=%p\n"
		 "j=%p; &j=%p",
		 j,j->l->l->l, 
		 j ,&j);
    i->l->l->l = NULL;
    j->l->l->l = NULL;
    gras_assert(list_eq(i,j));

    list_free(j);
  }
  list_free(i);
  return no_error;
}

/**** PBIO *****/
typedef struct { /* structure presented in the IEEE article */
  int Cnstatv;
  double Cstatev[12];
  int Cnprops;
  double Cprops[110];
  int Cndi[4], Cnshr, Cnpt;
  double Cdtime, Ctime[2];
  int Cntens;
  double Cdfgrd0[3][373], Cdfgrd1[3][3], Cstress[106], Cddsdde[106][106];
} KSdata1;

int main(int argc,char *argv[]) {
  gras_error_t errcode;
  gras_socket_t *sock;
  int direction = RW; // READ; // WRITE

  gras_init_defaultlog(&argc,argv,
		       "DataDesc.thresh=verbose"
		       " test.thresh=debug"
		       //" set.thresh=debug"
		       );
  if (argc >= 2) {
    if (!strcmp(argv[1], "--read"))
      direction = READ;
    if (!strcmp(argv[1], "--write"))
      direction = WRITE;
  }
    
  if (direction == WRITE) 
    TRYFAIL(gras_socket_client_from_file("datadesc_usage.out",&sock));
  if (direction == READ) 
    TRYFAIL(gras_socket_server_from_file("datadesc_usage.out",&sock));
  
  TRYFAIL(test_int(sock,direction));    
  TRYFAIL(test_float(sock,direction));  
  TRYFAIL(test_array(sock,direction));  
  TRYFAIL(test_intref(sock,direction)); 
  
  TRYFAIL(test_string(sock,direction)); 

  TRYFAIL(test_homostruct(sock,direction));
  TRYFAIL(test_hetestruct(sock,direction));
  TRYFAIL(test_nestedstruct(sock,direction));

  TRYFAIL(declare_chained_list_type());
  TRYFAIL(test_chain_list(sock,direction));
  TRYFAIL(test_graph(sock,direction));

  if (direction != RW) 
    gras_socket_close(&sock);
  gras_exit();
  return 0;
}
