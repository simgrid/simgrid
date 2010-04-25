/* datadesc: test of data description (using file transport).               */

/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include <stdio.h>
#include "gras.h"

#include "gras/DataDesc/datadesc_interface.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Logging specific to this test");

#define READ  0
#define WRITE 1
#define COPY  2

const char *filename = "datadesc_usage.out";

void
write_read(const char *msgtype, void *src, void *dst,
           gras_socket_t sock, int direction);

void
write_read(const char *msgtype, void *src, void *dst,
           gras_socket_t sock, int direction)
{

  switch (direction) {
  case READ:
    gras_msg_wait(15, msgtype, NULL, dst);
    break;

  case WRITE:
    gras_msg_send(sock, msgtype, src);
    break;

  case COPY:
    gras_datadesc_memcpy(gras_datadesc_by_name(msgtype), src, dst);
    break;
  }
}


/* defined in datadesc_structures.c, which in perl generated */
void register_structures(void);
void test_structures(gras_socket_t sock, int direction);

/************************
 * Each and every tests *
 ************************/

static void test_int(gras_socket_t sock, int direction)
{
  int i = 5, j;

  INFO0("---- Test on integer ----");
  write_read("int", &i, &j, sock, direction);
  if (direction == READ || direction == COPY)
    xbt_assert(i == j);
}

static void test_float(gras_socket_t sock, int direction)
{
  float i = 5.0, j;

  INFO0("---- Test on float ----");
  write_read("float", &i, &j, sock, direction);
  if (direction == READ || direction == COPY)
    xbt_assert2(i == j, "%f != %f", i, j);
}

static void test_double(gras_socket_t sock, int direction)
{
  double i = -3252355.1234, j;

  INFO0("---- Test on double ----");
  write_read("double", &i, &j, sock, direction);
  if (direction == READ || direction == COPY)
    xbt_assert2(i == j, "%f != %f", i, j);
}

#define FIXED_ARRAY_SIZE 5
typedef int array[FIXED_ARRAY_SIZE];
static void test_array(gras_socket_t sock, int direction)
{
  array i = { 35212, -6226, 74337, 11414, 7733 };
  array j;
  int cpt;

  INFO0("---- Test on fixed array ----");

  write_read("fixed int array", &i, &j, sock, direction);
  if (direction == READ || direction == COPY) {
    for (cpt = 0; cpt < FIXED_ARRAY_SIZE; cpt++) {
      DEBUG1("Test spot %d", cpt);
      xbt_assert4(i[cpt] == j[cpt], "i[%d]=%d  !=  j[%d]=%d",
                  cpt, i[cpt], cpt, j[cpt]);
    }
  }
}

/*** Dynar of scalar ***/

static void test_dynar_scal(gras_socket_t sock, int direction)
{
  xbt_dynar_t i, j;
  int cpt;

  INFO0("---- Test on dynar containing integers ----");
  i = xbt_dynar_new(sizeof(int), NULL);
  for (cpt = 0; cpt < 64; cpt++) {
    xbt_dynar_push_as(i, int, cpt);
    DEBUG2("Push %d, length=%lu", cpt, xbt_dynar_length(i));
  }
  /*  xbt_dynar_dump(i); */
  write_read("xbt_dynar_of_int", &i, &j, sock, direction);
  /*  xbt_dynar_dump(j); */
  if (direction == READ || direction == COPY) {
    for (cpt = 0; cpt < 64; cpt++) {
      int ret = xbt_dynar_get_as(j, cpt, int);
      if (cpt != ret) {
        CRITICAL3
          ("The retrieved value for cpt=%d is not the same than the injected one (%d!=%d)",
           cpt, ret, cpt);
        xbt_abort();
      }
    }
    xbt_dynar_free(&j);
  }
  xbt_dynar_free(&i);
}

/*** Empty dynar ***/

static void test_dynar_empty(gras_socket_t sock, int direction)
{
  xbt_dynar_t i, j;

  INFO0("---- Test on empty dynar of integers ----");
  i = xbt_dynar_new(sizeof(int), NULL);
  write_read("xbt_dynar_of_int", &i, &j, sock, direction);
  /*  xbt_dynar_dump(j); */
  if (direction == READ || direction == COPY) {
    xbt_assert(xbt_dynar_length(j) == 0);
    xbt_dynar_free(&j);
  }
  xbt_dynar_free(&i);
}

static void test_intref(gras_socket_t sock, int direction)
{
  int *i, *j;

  i = xbt_new(int, 1);
  *i = 12345;

  INFO0("---- Test on a reference to an integer ----");

  write_read("int*", &i, &j, sock, direction);
  if (direction == READ || direction == COPY) {
    xbt_assert2(*i == *j, "*i != *j (%d != %d)", *i, *j);
    free(j);
  }
  free(i);
}

/***
 *** string (dynamic array)
 ***/
static void test_string(gras_socket_t sock, int direction)
{
  char *i = xbt_strdup("Some data"), *j = NULL;
  int cpt;

  INFO0("---- Test on string (ref to dynamic array) ----");
  write_read("string", &i, &j, sock, direction);
  if (direction == READ || direction == COPY) {
    for (cpt = 0; cpt < strlen(i); cpt++) {
      xbt_assert4(i[cpt] == j[cpt], "i[%d]=%c  !=  j[%d]=%c",
                  cpt, i[cpt], cpt, j[cpt]);
    }
    free(j);
  }
  free(i);
}


/***
 *** homogeneous struct
 ***/
typedef struct {
  int a, b, c, d;
} homostruct;
static void test_homostruct(gras_socket_t sock, int direction)
{
  homostruct *i, *j;

  INFO0("---- Test on homogeneous structure ----");
  /* init a value, exchange it and check its validity */
  i = xbt_new(homostruct, 1);
  i->a = 2235;
  i->b = 433425;
  i->c = -23423;
  i->d = -235235;

  write_read("homostruct*", &i, &j, sock, direction);
  if (direction == READ || direction == COPY) {
    xbt_assert2(i->a == j->a, "i->a=%d != j->a=%d", i->a, j->a);
    xbt_assert(i->b == j->b);
    xbt_assert(i->c == j->c);
    xbt_assert(i->d == j->d);
    free(j);
  }
  free(i);
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
static void test_hetestruct(gras_socket_t sock, int direction)
{
  hetestruct *i, *j;

  INFO0("---- Test on heterogeneous structure ----");
  /* init a value, exchange it and check its validity */
  i = xbt_new(hetestruct, 1);
  i->c1 = 's';
  i->l1 = 123455;
  i->c2 = 'e';
  i->l2 = 774531;

  write_read("hetestruct*", &i, &j, sock, direction);
  if (direction == READ || direction == COPY) {
    xbt_assert(i->c1 == j->c1);
    xbt_assert(i->c2 == j->c2);
    xbt_assert2(i->l1 == j->l1, "i->l1(=%ld)  !=  j->l1(=%ld)", i->l1, j->l1);
    xbt_assert(i->l2 == j->l2);
    free(j);
  }
  free(i);
}

static void test_hetestruct_array(gras_socket_t sock, int direction)
{
  hetestruct *i, *j, *p, *q;
  int cpt;

  INFO0("---- Test on heterogeneous structure arrays ----");
  /* init a value, exchange it and check its validity */
  i = xbt_malloc(sizeof(hetestruct) * 10);
  for (cpt = 0, p = i; cpt < 10; cpt++, p++) {
    p->c1 = 's' + cpt;
    p->l1 = 123455 + cpt;
    p->c2 = 'e' + cpt;
    p->l2 = 774531 + cpt;
  }

  write_read("hetestruct[10]*", &i, &j, sock, direction);
  if (direction == READ || direction == COPY) {
    for (cpt = 0, p = i, q = j; cpt < 10; cpt++, p++, q++) {
      xbt_assert(p->c1 == q->c1);
      xbt_assert(p->c2 == q->c2);
      xbt_assert3(p->l1 == p->l1,
                  "for cpt=%d i->l1(=%ld)  !=  j->l1(=%ld)", cpt, p->l1,
                  q->l1);
      xbt_assert(q->l2 == p->l2);
    }
    free(j);
  }
  free(i);
}

/***
 *** nested struct
 ***/
typedef struct {
  hetestruct hete;
  homostruct homo;
} nestedstruct;
static void test_nestedstruct(gras_socket_t sock, int direction)
{
  nestedstruct *i, *j;

  INFO0("---- Test on nested structures ----");
  /* init a value, exchange it and check its validity */
  i = xbt_new(nestedstruct, 1);
  i->homo.a = 235231;
  i->homo.b = -124151;
  i->homo.c = 211551;
  i->homo.d = -664222;
  i->hete.c1 = 's';
  i->hete.l1 = 123455;
  i->hete.c2 = 'e';
  i->hete.l2 = 774531;

  write_read("nestedstruct*", &i, &j, sock, direction);
  if (direction == READ || direction == COPY) {
    xbt_assert(i->homo.a == j->homo.a);
    xbt_assert(i->homo.b == j->homo.b);
    xbt_assert(i->homo.c == j->homo.c);
    xbt_assert(i->homo.d == j->homo.d);
    xbt_assert(i->hete.c1 == j->hete.c1);
    xbt_assert(i->hete.c2 == j->hete.c2);
    xbt_assert(i->hete.l1 == j->hete.l1);
    xbt_assert(i->hete.l2 == j->hete.l2);
    free(j);
  }
  free(i);
}

/***
 *** chained list
 ***/
typedef struct s_chained_list chained_list_t;
struct s_chained_list {
  int v;
  chained_list_t *l;
};
chained_list_t *cons(int v, chained_list_t * l);
void list_free(chained_list_t * l);
int list_eq(chained_list_t * i, chained_list_t * j);

chained_list_t *cons(int v, chained_list_t * l)
{
  chained_list_t *nl = xbt_new(chained_list_t, 1);

  nl->v = v;
  nl->l = l;

  return nl;
}

void list_free(chained_list_t * l)
{
  if (l) {
    list_free(l->l);
    free(l);
  }
}

int list_eq(chained_list_t * i, chained_list_t * j)
{
  if (!i || !j)
    return i == j;
  if (i->v != j->v)
    return 0;
  return list_eq(i->l, j->l);
}

static void test_chain_list(gras_socket_t sock, int direction)
{
  chained_list_t *i, *j;

  INFO0("---- Test on chained list ----");

  /* init a value, exchange it and check its validity */
  i = cons(12355, cons(246264, cons(23263, NULL)));
  j = NULL;

  write_read("chained_list_t*", &i, &j, sock, direction);
  if (direction == READ || direction == COPY) {
    xbt_assert(list_eq(i, j));
    list_free(j);
  }

  list_free(i);
}

/***
 *** graph
 ***/
static void test_graph(gras_socket_t sock, int direction)
{
  chained_list_t *i, *j;

  INFO0("---- Test on graph (cyclique chained list of 3 items) ----");
  /* init a value, exchange it and check its validity */
  i = cons(1151515, cons(-232362, cons(222552, NULL)));
  i->l->l->l = i;
  j = NULL;

  write_read("chained_list_t*", &i, &j, sock, direction);
  if (direction == READ || direction == COPY) {

    DEBUG1("i=%p", i);
    DEBUG1("i->l=%p", i->l);
    DEBUG1("i->l->l=%p", i->l->l);
    DEBUG1("i->l->l->l=%p", i->l->l->l);
    DEBUG1("j=%p", j);
    DEBUG1("j->l=%p", j->l);
    DEBUG1("j->l->l=%p", j->l->l);
    DEBUG1("j->l->l->l=%p", j->l->l->l);
    xbt_assert4(j->l->l->l == j,
                "Received list is not cyclic. j=%p != j->l->l->l=%p\n"
                "j=%p; &j=%p", j, j->l->l->l, j, &j);
    j->l->l->l = NULL;
    i->l->l->l = NULL;
    xbt_assert(list_eq(i, j));

    list_free(j);
  }
  i->l->l->l = NULL;            /* do this even in WRITE mode */
  list_free(i);
}


/*** Dynar of references ***/
static void free_string(void *d)
{                               /* used to free the data in dynar */
  free(*(void **) d);
}

static void test_dynar_ref(gras_socket_t sock, int direction)
{
  xbt_dynar_t i, j;
  char buf[1024];
  char *s1, *s2;
  int cpt;

  INFO0("---- Test on dynar containing integers ----");

  i = xbt_dynar_new(sizeof(char *), &free_string);
  for (cpt = 0; cpt < 64; cpt++) {
    sprintf(buf, "%d", cpt);
    s1 = strdup(buf);
    xbt_dynar_push(i, &s1);
  }

  write_read("xbt_dynar_of_string", &i, &j, sock, direction);
  if (direction == READ || direction == COPY) {
    for (cpt = 0; cpt < 64; cpt++) {
      sprintf(buf, "%d", cpt);
      xbt_dynar_shift(j, &s2);
      xbt_assert2(!strcmp(buf, s2),
                  "The retrieved value is not the same than the injected one (%s!=%s)",
                  buf, s2);
      free(s2);
    }
    xbt_dynar_free(&j);
  }
  xbt_dynar_free(&i);
}


/**** PBIO *****/
GRAS_DEFINE_TYPE(s_pbio, struct s_pbio {        /* structure presented in the IEEE article */
                 int Cnstatv; double Cstatev[12];
                 int Cnprops;
                 double Cprops[110]; int Cndi[4]; int Cnshr; int Cnpt;
                 double Cdtime;
                 double Ctime[2];
                 int Cntens; double Cdfgrd0[373][3]; double Cdfgrd1[3][3];
                 double Cstress[106]; double Cddsdde[106][106];};)

     typedef struct s_pbio pbio_t;

     static void test_pbio(gras_socket_t sock, int direction)
{
  int cpt;
  int cpt2;
  gras_datadesc_type_t pbio_type;
  pbio_t i, j;

  INFO0
    ("---- Test on the PBIO IEEE struct (also tests GRAS DEFINE TYPE) ----");
  pbio_type = gras_datadesc_by_symbol(s_pbio);

  /* Fill in that damn struct */
  i.Cnstatv = 325115;
  for (cpt = 0; cpt < 12; cpt++)
    i.Cstatev[cpt] = ((double) cpt) * -2361.11;
  i.Cnprops = -37373;
  for (cpt = 0; cpt < 110; cpt++)
    i.Cprops[cpt] = cpt * 100.0;
  for (cpt = 0; cpt < 4; cpt++)
    i.Cndi[cpt] = cpt * 23262;
  i.Cnshr = -4634;
  i.Cnpt = 114142;
  i.Cdtime = -11515.662;
  i.Ctime[0] = 332523.226;
  i.Ctime[1] = -26216.113;
  i.Cntens = 235211411;

  for (cpt = 0; cpt < 3; cpt++) {
    for (cpt2 = 0; cpt2 < 373; cpt2++)
      i.Cdfgrd0[cpt2][cpt] = ((double) cpt) * ((double) cpt2);
    for (cpt2 = 0; cpt2 < 3; cpt2++)
      i.Cdfgrd1[cpt][cpt2] = -((double) cpt) * ((double) cpt2);
  }
  for (cpt = 0; cpt < 106; cpt++) {
    i.Cstress[cpt] = (double) cpt *22.113;
    for (cpt2 = 0; cpt2 < 106; cpt2++)
      i.Cddsdde[cpt][cpt2] = ((double) cpt) * ((double) cpt2);
  }
  write_read("s_pbio", &i, &j, sock, direction);
  if (direction == READ || direction == COPY) {
    /* Check that the data match */
    xbt_assert(i.Cnstatv == j.Cnstatv);
    for (cpt = 0; cpt < 12; cpt++)
      xbt_assert4(i.Cstatev[cpt] == j.Cstatev[cpt],
                  "i.Cstatev[%d] (=%f) != j.Cstatev[%d] (=%f)",
                  cpt, i.Cstatev[cpt], cpt, j.Cstatev[cpt]);
    xbt_assert(i.Cnprops == j.Cnprops);
    for (cpt = 0; cpt < 110; cpt++)
      xbt_assert(i.Cprops[cpt] == j.Cprops[cpt]);
    for (cpt = 0; cpt < 4; cpt++)
      xbt_assert(i.Cndi[cpt] == j.Cndi[cpt]);
    xbt_assert(i.Cnshr == j.Cnshr);
    xbt_assert(i.Cnpt == j.Cnpt);
    xbt_assert(i.Cdtime == j.Cdtime);
    xbt_assert(i.Ctime[0] == j.Ctime[0]);
    xbt_assert(i.Ctime[1] == j.Ctime[1]);
    xbt_assert(i.Cntens == j.Cntens);
    for (cpt = 0; cpt < 3; cpt++) {
      for (cpt2 = 0; cpt2 < 373; cpt2++)
        xbt_assert(i.Cdfgrd0[cpt2][cpt] == j.Cdfgrd0[cpt2][cpt]);
      for (cpt2 = 0; cpt2 < 3; cpt2++)
        xbt_assert(i.Cdfgrd1[cpt][cpt2] == j.Cdfgrd1[cpt][cpt2]);
    }
    for (cpt = 0; cpt < 106; cpt++) {
      xbt_assert(i.Cstress[cpt] == j.Cstress[cpt]);
      for (cpt2 = 0; cpt2 < 106; cpt2++)
        xbt_assert4(i.Cddsdde[cpt][cpt2] == j.Cddsdde[cpt][cpt2],
                    "%f=i.Cddsdde[%d][%d] != j.Cddsdde[cpt][cpt2]=%f",
                    i.Cddsdde[cpt][cpt2], cpt, cpt2, j.Cddsdde[cpt][cpt2]);
    }
  }
}

GRAS_DEFINE_TYPE(s_clause, struct s_clause {
                 int num_lits; int *literals GRAS_ANNOTE(size, num_lits);       /* Tells GRAS where to find the size */
                 };

  )

     typedef struct s_clause Clause;

     static void test_clause(gras_socket_t sock, int direction)
{
  Clause *i, *j;
  int cpt;

  INFO0
    ("---- Test on struct containing dynamic array and its size (cbps test) ----");

  /* create and fill the struct */
  i = xbt_new(Clause, 1);

  i->num_lits = 5432;
  i->literals = xbt_new(int, i->num_lits);
  for (cpt = 0; cpt < i->num_lits; cpt++)
    i->literals[cpt] = cpt * cpt - ((cpt * cpt) / 2);
  DEBUG3("created data=%p (within %p @%p)", &(i->num_lits), i, &i);
  DEBUG1("created count=%d", i->num_lits);

  write_read("Clause*", &i, &j, sock, direction);
  if (direction == READ || direction == COPY) {
    xbt_assert(i->num_lits == j->num_lits);
    for (cpt = 0; cpt < i->num_lits; cpt++)
      xbt_assert(i->literals[cpt] == j->literals[cpt]);

    free(j->literals);
    free(j);
  }
  free(i->literals);
  free(i);
}

static void test_clause_empty(gras_socket_t sock, int direction)
{
  Clause *i, *j;

  INFO0
    ("---- Test on struct containing dynamic array and its size when size=0 (cbps test) ----");

  /* create and fill the struct */
  i = xbt_new(Clause, 1);

  i->num_lits = 0;
  i->literals = NULL;
  DEBUG3("created data=%p (within %p @%p)", &(i->num_lits), i, &i);
  DEBUG1("created count=%d", i->num_lits);

  write_read("Clause*", &i, &j, sock, direction);
  if (direction == READ || direction == COPY) {
    xbt_assert(i->num_lits == j->num_lits);

    free(j->literals);
    free(j);
  }
  free(i->literals);
  free(i);
}

static void register_types(void)
{
  gras_datadesc_type_t my_type, ref_my_type;

  gras_msgtype_declare("char", gras_datadesc_by_name("char"));
  gras_msgtype_declare("int", gras_datadesc_by_name("int"));
  gras_msgtype_declare("float", gras_datadesc_by_name("float"));
  gras_msgtype_declare("double", gras_datadesc_by_name("double"));

  my_type = gras_datadesc_array_fixed("fixed int array",
                                      gras_datadesc_by_name("int"),
                                      FIXED_ARRAY_SIZE);
  gras_msgtype_declare("fixed int array", my_type);

  my_type = gras_datadesc_dynar(gras_datadesc_by_name("int"), NULL);
  gras_msgtype_declare("xbt_dynar_of_int", my_type);

  my_type = gras_datadesc_ref("int*", gras_datadesc_by_name("int"));
  gras_msgtype_declare("int*", my_type);

  gras_msgtype_declare("string", gras_datadesc_by_name("string"));

  my_type = gras_datadesc_struct("homostruct");
  gras_datadesc_struct_append(my_type, "a",
                              gras_datadesc_by_name("signed int"));
  gras_datadesc_struct_append(my_type, "b", gras_datadesc_by_name("int"));
  gras_datadesc_struct_append(my_type, "c", gras_datadesc_by_name("int"));
  gras_datadesc_struct_append(my_type, "d", gras_datadesc_by_name("int"));
  gras_datadesc_struct_close(my_type);
  my_type =
    gras_datadesc_ref("homostruct*", gras_datadesc_by_name("homostruct"));
  gras_msgtype_declare("homostruct*", my_type);

  my_type = gras_datadesc_struct("hetestruct");
  gras_datadesc_struct_append(my_type, "c1",
                              gras_datadesc_by_name("unsigned char"));
  gras_datadesc_struct_append(my_type, "l1",
                              gras_datadesc_by_name("unsigned long int"));
  gras_datadesc_struct_append(my_type, "c2",
                              gras_datadesc_by_name("unsigned char"));
  gras_datadesc_struct_append(my_type, "l2",
                              gras_datadesc_by_name("unsigned long int"));
  gras_datadesc_struct_close(my_type);
  my_type =
    gras_datadesc_ref("hetestruct*", gras_datadesc_by_name("hetestruct"));
  gras_msgtype_declare("hetestruct*", my_type);

  my_type =
    gras_datadesc_array_fixed("hetestruct[10]",
                              gras_datadesc_by_name("hetestruct"), 10);
  my_type = gras_datadesc_ref("hetestruct[10]*", my_type);
  gras_msgtype_declare("hetestruct[10]*", my_type);

  my_type = gras_datadesc_struct("nestedstruct");
  gras_datadesc_struct_append(my_type, "hete",
                              gras_datadesc_by_name("hetestruct"));
  gras_datadesc_struct_append(my_type, "homo",
                              gras_datadesc_by_name("homostruct"));
  gras_datadesc_struct_close(my_type);
  my_type =
    gras_datadesc_ref("nestedstruct*", gras_datadesc_by_name("nestedstruct"));
  gras_msgtype_declare("nestedstruct*", my_type);

  my_type = gras_datadesc_struct("chained_list_t");
  ref_my_type = gras_datadesc_ref("chained_list_t*", my_type);
  gras_datadesc_struct_append(my_type, "v", gras_datadesc_by_name("int"));
  gras_datadesc_struct_append(my_type, "l", ref_my_type);
  gras_datadesc_struct_close(my_type);
  gras_datadesc_cycle_set(gras_datadesc_by_name("chained_list_t*"));
  gras_msgtype_declare("chained_list_t", my_type);
  gras_msgtype_declare("chained_list_t*", ref_my_type);

  my_type =
    gras_datadesc_dynar(gras_datadesc_by_name("string"), &free_string);
  gras_msgtype_declare("xbt_dynar_of_string", my_type);

  my_type = gras_datadesc_by_symbol(s_pbio);
  gras_msgtype_declare("s_pbio", my_type);

  my_type = gras_datadesc_by_symbol(s_clause);
  my_type = gras_datadesc_ref("Clause*", my_type);
  gras_msgtype_declare("Clause*", my_type);

}

#ifdef __BORLANDC__
#pragma argsused
#endif

int main(int argc, char *argv[])
{
  gras_socket_t sock = NULL;
  int direction = WRITE;
  int cpt;
  char local_arch, remote_arch;

  gras_init(&argc, argv);
  register_types();
  register_structures();

  for (cpt = 1; cpt < argc; cpt++) {
    if (!strcmp(argv[cpt], "--arch")) {
      INFO2("We are on %s (#%d)",
            gras_datadesc_arch_name(gras_arch_selfid()),
            (int) gras_arch_selfid());
      exit(0);
    } else if (!strcmp(argv[cpt], "--help")) {
      printf("Usage: datadesc_usage [option] [filename]\n");
      printf(" --arch: display the current architecture and quit.\n");
      printf(" --read file: reads the description from the given file\n");
      printf(" --write file: writes the description to the given file\n");
      printf(" --copy: copy the description in memory\n");
      printf
        (" --regen: write the description to the file of the current architecture\n");
      printf(" --help: displays this message\n");
      exit(0);
    } else if (!strcmp(argv[cpt], "--regen")) {
      direction = WRITE;
      filename =
        bprintf("datadesc.%s", gras_datadesc_arch_name(gras_arch_selfid()));
    } else if (!strcmp(argv[cpt], "--read")) {
      direction = READ;
    } else if (!strcmp(argv[cpt], "--write")) {
      direction = WRITE;
    } else if (!strcmp(argv[cpt], "--copy")) {
      direction = COPY;
    } else {
      filename = argv[cpt];
    }
  }

  if (direction == WRITE) {
    INFO1("Write to file %s", strrchr(filename,'/')?strrchr(filename,'/')+1:filename);
    sock = gras_socket_client_from_file(filename);
  }
  if (direction == READ) {
    INFO1("Read from file %s", strrchr(filename,'/')?strrchr(filename,'/')+1:filename);
    sock = gras_socket_server_from_file(filename);
  }
  if (direction == COPY) {
    INFO0("Memory copy");
  }

  local_arch = gras_arch_selfid();
  write_read("char", &local_arch, &remote_arch, sock, direction);
  if (direction == READ)
    VERB2("This file was generated on %s (%d)",
          gras_datadesc_arch_name(remote_arch), (int) remote_arch);


  test_int(sock, direction);
  test_float(sock, direction);
  test_double(sock, direction);
  test_array(sock, direction);
  test_intref(sock, direction);

  test_string(sock, direction);
  test_dynar_scal(sock, direction);
  test_dynar_empty(sock, direction);

  test_structures(sock, direction);

  test_homostruct(sock, direction);
  test_hetestruct(sock, direction);
  test_nestedstruct(sock, direction);
  test_hetestruct_array(sock, direction);

  test_chain_list(sock, direction);
  test_graph(sock, direction);
  test_dynar_ref(sock, direction);

  test_pbio(sock, direction);

  test_clause(sock, direction);
  test_clause_empty(sock, direction);

  if (direction != COPY)
    gras_socket_close(sock);

  gras_exit();
  return 0;
}
