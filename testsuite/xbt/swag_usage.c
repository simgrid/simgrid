#include <stdlib.h>
#include <stdio.h>
#include "swag.h"

typedef struct {
  s_xbt_swag_hookup_t setA;
  s_xbt_swag_hookup_t setB;
  char *name;
} shmurtz, s_shmurtz_t, *shmurtz_t;

int main(void)
{
  shmurtz_t obj1, obj2, obj;
  xbt_swag_t setA,setB;

  obj1 = calloc(1,sizeof(s_shmurtz_t));
  obj2 = calloc(1,sizeof(s_shmurtz_t));

  obj1->name="Obj 1";
  obj2->name="Obj 2";

  printf("%p %p %d\n",obj1,&(obj1->setB),
	 (char *)&(obj1->setB) - (char *)obj1);

  setA = xbt_swag_new((char *)&(obj1->setA) - (char *)obj1);
  setB = xbt_swag_new((char *)&(obj1->setB) - (char *)obj1);

  xbt_swag_insert(obj1, setA);
  xbt_swag_insert(obj1, setB);
  xbt_swag_insert(obj2, setA);
  xbt_swag_insert(obj2, setB);

  xbt_swag_extract(obj1, setB);
  //  xbt_swag_extract(obj2, setB);

  xbt_swag_foreach(obj,setA) {
    printf("\t%s\n",obj->name);
  }

  xbt_swag_foreach(obj,setB) {
    printf("\t%s\n",obj->name);
  }

  printf("Belongs : %d\n", xbt_swag_belongs(obj2,setB));

  printf("%d %d\n", xbt_swag_size(setA),xbt_swag_size(setB));
  return 0;
}
