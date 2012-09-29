#include "../mc_private.h"

static void test1(void);
static void test2(void);
static void test2(void);
static void test4(void);
static void test5(void);
static void test6(void);

static void test1()
{

  fprintf(stderr, "\n**************** TEST 1 ****************\n\n");
  MC_SET_RAW_MEM;

  /* Save first snapshot */
  mc_snapshot_t snapshot1 = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(snapshot1);

  /* Save second snapshot */
  mc_snapshot_t snapshot2 = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(snapshot2);

  int res = snapshot_compare(snapshot1, snapshot2);
  xbt_assert(res == 0);

  MC_UNSET_RAW_MEM;

  fprintf(stderr, "\n**************** END TEST 1 ****************\n");

  MC_SET_RAW_MEM;
  
  MC_free_snapshot(snapshot1);
  MC_free_snapshot(snapshot2);

  MC_UNSET_RAW_MEM;
}

static void test2()
{

  fprintf(stderr, "\n**************** TEST 2 ****************\n\n");

  MC_SET_RAW_MEM;

  /* Save first snapshot */
  mc_snapshot_t snapshot1 = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(snapshot1);

  MC_UNSET_RAW_MEM;

  char* t = malloc(50);
  t = strdup("toto");
 
  MC_SET_RAW_MEM;

  /* Save second snapshot */
  mc_snapshot_t snapshot2 = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(snapshot2);

  int res = snapshot_compare(snapshot1, snapshot2);
  xbt_assert(res != 0);

  MC_UNSET_RAW_MEM;
  
  fprintf(stderr, "\n**************** END TEST 2 ****************\n");

  free(t);

  MC_SET_RAW_MEM;

  MC_free_snapshot(snapshot1);
  MC_free_snapshot(snapshot2);

  MC_UNSET_RAW_MEM;
}

static void test3()
{

  fprintf(stderr, "\n**************** TEST 3 ****************\n\n");

  MC_SET_RAW_MEM;

  /* Save first snapshot */
  mc_snapshot_t snapshot1 = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(snapshot1);

  MC_UNSET_RAW_MEM;

  char *t = malloc(5);
  t = strdup("toto");
  free(t);

  MC_SET_RAW_MEM;

  /* Save second snapshot */
  mc_snapshot_t snapshot2 = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(snapshot2);

  int res = snapshot_compare(snapshot1, snapshot2);
  xbt_assert(res == 0);
  
  MC_UNSET_RAW_MEM;
  
  fprintf(stderr, "\n**************** END TEST 3 ****************\n");

  MC_SET_RAW_MEM;

  MC_free_snapshot(snapshot1);
  MC_free_snapshot(snapshot2);

  MC_UNSET_RAW_MEM;
}

static void test4()
{

  fprintf(stderr, "\n**************** TEST 4 ****************\n\n");

  char *t = malloc(5);
  t = strdup("toto");

  MC_SET_RAW_MEM;

  /* Save first snapshot */
  mc_snapshot_t snapshot1 = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(snapshot1);

  MC_UNSET_RAW_MEM;

  free(t);

  MC_SET_RAW_MEM;

  /* Save second snapshot */
  mc_snapshot_t snapshot2 = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(snapshot2);

  int res = snapshot_compare(snapshot1, snapshot2);
  xbt_assert(res != 0);
  
  MC_UNSET_RAW_MEM;
  
  fprintf(stderr, "\n**************** END TEST 4 ****************\n");
  
  MC_SET_RAW_MEM;

  MC_free_snapshot(snapshot1);
  MC_free_snapshot(snapshot2);

  MC_UNSET_RAW_MEM;
}


static void test5()
{

  fprintf(stderr, "\n**************** TEST 5 ****************\n\n");

  char *ptr1 = malloc(sizeof(char *));

  MC_SET_RAW_MEM;

  /* Save first snapshot */
  mc_snapshot_t snapshot1 = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(snapshot1);

  MC_UNSET_RAW_MEM;

  *ptr1 = *ptr1 + 1;

  MC_SET_RAW_MEM;

  /* Save second snapshot */
  mc_snapshot_t snapshot2 = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(snapshot2);

  int res = snapshot_compare(snapshot1, snapshot2);
  xbt_assert(res != 0);
  
  MC_UNSET_RAW_MEM;
  
  fprintf(stderr, "\n**************** END TEST 5 ****************\n");

  MC_SET_RAW_MEM;

  MC_free_snapshot(snapshot1);
  MC_free_snapshot(snapshot2);

  MC_UNSET_RAW_MEM;
}

static void test6()
{

  fprintf(stderr, "\n**************** TEST 6 ****************\n\n");

  char *ptr1 = malloc(sizeof(char *));

  MC_SET_RAW_MEM;

  /* Save first snapshot */
  mc_snapshot_t snapshot1 = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(snapshot1);

  MC_UNSET_RAW_MEM;

  *ptr1 = *ptr1 + 1;
  *ptr1 = *ptr1 - 1;

  MC_SET_RAW_MEM;

  /* Save second snapshot */
  mc_snapshot_t snapshot2 = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(snapshot2);

  int res = snapshot_compare(snapshot1, snapshot2);
  xbt_assert(res == 0);
  
  MC_UNSET_RAW_MEM;
  
  fprintf(stderr, "\n**************** END TEST 6 ****************\n");

  MC_SET_RAW_MEM;

  MC_free_snapshot(snapshot1);
  MC_free_snapshot(snapshot2);

  MC_UNSET_RAW_MEM;
}


void MC_test_snapshot_comparison(){

  MC_memory_init();

  MC_SET_RAW_MEM;
  mc_snapshot_t initial = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(initial); 
  MC_UNSET_RAW_MEM;

  /* Get .plt section (start and end addresses) for data libsimgrid comparison */
  get_plt_section();

  test1();

  MC_restore_snapshot(initial);
  MC_UNSET_RAW_MEM;
  
  test2();
  
  MC_restore_snapshot(initial);
  MC_UNSET_RAW_MEM;

  test3();

  MC_restore_snapshot(initial);
  MC_UNSET_RAW_MEM;

  test4();

  MC_restore_snapshot(initial);
  MC_UNSET_RAW_MEM;
  
  test5();

  MC_restore_snapshot(initial);
  MC_UNSET_RAW_MEM;
  
  test6();

  MC_restore_snapshot(initial);
  MC_UNSET_RAW_MEM;
}
