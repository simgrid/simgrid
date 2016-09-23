#include "xbt/mallocator.h"
#include "xbt.h"

typedef struct element {
  int value;
} s_element_t;
typedef struct element *element_t;

element_t base_mallocator;

static void *element_mallocator_new_f(void)
{
  element_t elem = xbt_new(s_element_t, 1);
  elem->value = 0;
  return elem;
}

static void element_mallocator_free_f(void *elem)
{
  xbt_free(elem);
}

#define element_mallocator_reset_f ((void_f_pvoid_t)NULL)

static void pprint_elems(xbt_dynar_t elems) {
  unsigned int iter;
  element_t elem;
  printf("Elems:");
  xbt_dynar_foreach(elems, iter, elem) {
    printf(" (%d,%d)", elem->value, (int)(base_mallocator-elem)/(int)sizeof(int));
  }
  printf("\n");
}

int main(int argc, char**argv)
{
  xbt_mallocator_initialization_is_done(1);
  int i = 0;
  xbt_mallocator_t mallocator =
    xbt_mallocator_new(65536, element_mallocator_new_f, element_mallocator_free_f, element_mallocator_reset_f);
  xbt_dynar_t elems = xbt_dynar_new(sizeof(element_t), NULL);
  element_t elem = NULL;
  base_mallocator = xbt_mallocator_get(mallocator);

  for (i=0; i<=10; i++) {
    elem = xbt_mallocator_get(mallocator);
    elem->value = i;
    xbt_dynar_push(elems, &elem);
  }
  pprint_elems(elems);

  for (i=0; i<=5; i++) {
    xbt_dynar_pop(elems, &elem);
    xbt_mallocator_release(mallocator, elem);
  }
  pprint_elems(elems);

  xbt_dynar_remove_at(elems, 2, &elem);
  xbt_mallocator_release(mallocator, elem);
  pprint_elems(elems);

  for (i=11; i<=15; i++) {
    elem = xbt_mallocator_get(mallocator);
    elem->value = i;
    xbt_dynar_push(elems, &elem);
  }
  pprint_elems(elems);

  return 0;
}
