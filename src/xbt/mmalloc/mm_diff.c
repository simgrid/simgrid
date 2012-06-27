/* mm_diff - Memory snapshooting and comparison                             */

/* Copyright (c) 2008-2012. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex_interface.h" /* internals of backtrace setup */
#include "xbt/str.h"
#include "mc/mc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mm_diff, xbt,
                                "Logging specific to mm_diff in mmalloc");

extern char *xbt_binary_name;

void mmalloc_backtrace_display(xbt_mheap_t mdp, void *ptr){
  size_t block = BLOCK(ptr);
  int type;
  xbt_ex_t e;

  if ((char *) ptr < (char *) mdp->heapbase || block > mdp->heapsize) {
    fprintf(stderr,"Ouch, this pointer is not mine. I cannot display its backtrace. I refuse it to death!!\n");
    abort();
  }

  type = mdp->heapinfo[block].type;

  if (type != 0) {
    //fprintf(stderr,"Only full blocks are backtraced for now. Ignoring your request.\n");
    return;
  }
  if (mdp->heapinfo[block].busy_block.bt_size == 0) {
    fprintf(stderr,"No backtrace available for that block, sorry.\n");
    return;
  }

  memcpy(&e.bt,&(mdp->heapinfo[block].busy_block.bt),sizeof(void*)*XBT_BACKTRACE_SIZE);
  e.used = mdp->heapinfo[block].busy_block.bt_size;

  xbt_ex_setup_backtrace(&e);
  if (e.used == 0) {
    fprintf(stderr, "(backtrace not set)\n");
  } else if (e.bt_strings == NULL) {
    fprintf(stderr, "(backtrace not ready to be computed. %s)\n",xbt_binary_name?"Dunno why":"xbt_binary_name not setup yet");
  } else {
    int i;

    fprintf(stderr, "Backtrace of where the block %p was malloced (%d frames):\n",ptr,e.used);
    for (i = 0; i < e.used; i++)       /* no need to display "xbt_backtrace_display" */{
      fprintf(stderr,"%d",i);fflush(NULL);
      fprintf(stderr, "---> %s\n", e.bt_strings[i] + 4);
    }
  }
}


void mmalloc_backtrace_block_display(xbt_mheap_t mdp, size_t block){

  int type;
  xbt_ex_t e;

  type = mdp->heapinfo[block].type;

  if (type != 0) {
    fprintf(stderr,"Only full blocks are backtraced for now. Ignoring your request.\n");
    return;
  }
  if (mdp->heapinfo[block].busy_block.bt_size == 0) {
    fprintf(stderr,"No backtrace available for that block, sorry.\n");
    return;
  }

  memcpy(&e.bt,&(mdp->heapinfo[block].busy_block.bt),sizeof(void*)*XBT_BACKTRACE_SIZE);
  e.used = mdp->heapinfo[block].busy_block.bt_size;

  xbt_ex_setup_backtrace(&e);
  if (e.used == 0) {
    fprintf(stderr, "(backtrace not set)\n");
  } else if (e.bt_strings == NULL) {
    fprintf(stderr, "(backtrace not ready to be computed. %s)\n",xbt_binary_name?"Dunno why":"xbt_binary_name not setup yet");
  } else {
    int i;

    fprintf(stderr, "Backtrace of where the block %zu was malloced (%d frames):\n", block ,e.used);
    for (i = 0; i < e.used; i++)       /* no need to display "xbt_backtrace_display" */{
      fprintf(stderr,"%d",i);fflush(NULL);
      fprintf(stderr, "---> %s\n", e.bt_strings[i] + 4);
    }
  }
}

void mmalloc_backtrace_fragment_display(xbt_mheap_t mdp, size_t block, size_t frag){

  xbt_ex_t e;

  memcpy(&e.bt,&(mdp->heapinfo[block].busy_frag.bt[frag]),sizeof(void*)*XBT_BACKTRACE_SIZE);
  e.used = XBT_BACKTRACE_SIZE;

  xbt_ex_setup_backtrace(&e);
  if (e.used == 0) {
    fprintf(stderr, "(backtrace not set)\n");
  } else if (e.bt_strings == NULL) {
    fprintf(stderr, "(backtrace not ready to be computed. %s)\n",xbt_binary_name?"Dunno why":"xbt_binary_name not setup yet");
  } else {
    int i;

    fprintf(stderr, "Backtrace of where the fragment %zu in block %zu was malloced (%d frames):\n", frag, block ,e.used);
    for (i = 0; i < e.used; i++)       /* no need to display "xbt_backtrace_display" */{
      fprintf(stderr,"%d",i);fflush(NULL);
      fprintf(stderr, "---> %s\n", e.bt_strings[i] + 4);
    }
  }
}

int mmalloc_compare_heap(xbt_mheap_t mdp1, xbt_mheap_t mdp2){

  if(mdp1 == NULL && mdp2 == NULL){
    fprintf(stderr, "Malloc descriptors null\n");
    return 0;
  }

  int errors = mmalloc_compare_mdesc(mdp1, mdp2);

  return (errors > 0);

}

int mmalloc_compare_mdesc(struct mdesc *mdp1, struct mdesc *mdp2){

  int errors = 0;

  if(mdp1->heaplimit != mdp2->heaplimit){
    fprintf(stderr,"Different limit of valid info table indices\n");
    return 1;
  }

  void* s_heap = (char *)mmalloc_get_current_heap() - STD_HEAP_SIZE - getpagesize();

  void *heapbase1 = (char *)mdp1 + BLOCKSIZE;
  void *heapbase2 = (char *)mdp2 + BLOCKSIZE;

  void * breakval1 = (char *)mdp1 + ((char *)mdp1->breakval - (char *)s_heap);
  void * breakval2 = (char *)mdp2 + ((char *)mdp2->breakval - (char *)s_heap);

  size_t i, j;
  void *addr_block1, *addr_block2, *addr_frag1, *addr_frag2;
  size_t frag_size;

  i = 1;

  int k;
  int distance = 0;
  int total_distance = 0;

  int pointer_align;
  void *address_pointed1, *address_pointed2;

  int block_pointed1, block_pointed2, frag_pointed1, frag_pointed2;
  void *addr_block_pointed1, *addr_block_pointed2;

  /* Check busy blocks*/

  while(i < mdp1->heaplimit){

    if(mdp1->heapinfo[i].type != mdp2->heapinfo[i].type){
      fprintf(stderr,"Different type of block : %d - %d\n", mdp1->heapinfo[i].type, mdp2->heapinfo[i].type);
      errors++;
    }

    /* Get address of block i in each heap */
    addr_block1 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)heapbase1));
    xbt_assert(addr_block1 < breakval1, "Block address out of heap memory used");

    addr_block2 = ((void*) (((ADDR2UINT(i)) - 1) * BLOCKSIZE + (char*)heapbase2));
    xbt_assert(addr_block2 < breakval2, "Block address out of heap memory used");

    if(mdp1->heapinfo[i].type == 0){ /* busy large block */

      if(mdp1->heapinfo[i].busy_block.size != mdp2->heapinfo[i].busy_block.size){
        fprintf(stderr,"Different size of a large cluster : %zu - %zu\n", mdp1->heapinfo[i].busy_block.size, mdp2->heapinfo[i].busy_block.size); 
        fflush(NULL);
        errors++;
      }

      if(mdp1->heapinfo[i].busy_block.busy_size != mdp2->heapinfo[i].busy_block.busy_size){
        fprintf(stderr,"Different busy_size of a large cluster : %zu - %zu\n", mdp1->heapinfo[i].busy_block.busy_size, mdp2->heapinfo[i].busy_block.busy_size); 
        fflush(NULL);
        errors++;
      }

      /* Hamming distance on different blocks */
      distance = 0;


      for(k=0;k<mdp1->heapinfo[i].busy_block.busy_size;k++){

        if(memcmp(((char *)addr_block1) + k, ((char *)addr_block2) + k, 1) != 0){

          fprintf(stderr, "Different byte (offset=%d) (%p - %p) in block %zu\n", k, (char *)addr_block1 + k, (char *)addr_block2 + k, i); fflush(NULL);
          
          /* Check if pointer difference */
          pointer_align = (k >> sizeof(void*)) * sizeof(void*);
          address_pointed1 = *((void **)((char *)addr_block1 + pointer_align));
          address_pointed2 = *((void **)((char *)addr_block2 + pointer_align));

          fprintf(stderr, "Addresses pointed : %p - %p\n", address_pointed1, address_pointed2);
          
          block_pointed1 = ((char*)address_pointed1 - (char*)((struct mdesc*)s_heap)->heapbase) / BLOCKSIZE + 1;
          block_pointed2 = ((char*)address_pointed2 - (char*)((struct mdesc*)s_heap)->heapbase) / BLOCKSIZE + 1;
          
          fprintf(stderr, "Blocks pointed : %d - %d\n", block_pointed1, block_pointed2);
          
          if((char *) address_pointed1 < (char*)((struct mdesc*)s_heap)->heapbase || block_pointed1 > mdp1->heapsize || block_pointed1 < 1 || (char *) address_pointed2 < (char*)((struct mdesc*)s_heap)->heapbase || block_pointed2 > mdp2->heapsize || block_pointed2 < 1) {
            fprintf(stderr, "Unknown pointer ! \n");
            fflush(NULL);
            distance++;
            continue;
          }
          
          addr_block_pointed1 = ((void*) (((ADDR2UINT((size_t)block_pointed1)) - 1) * BLOCKSIZE + (char*)heapbase1));
          addr_block_pointed2 = ((void*) (((ADDR2UINT((size_t)block_pointed2)) - 1) * BLOCKSIZE + (char*)heapbase2));
          
          if(mdp1->heapinfo[block_pointed1].type == mdp2->heapinfo[block_pointed2].type){
            
            if(mdp1->heapinfo[block_pointed1].type == 0){ // Large block
              
              if(mdp1->heapinfo[block_pointed1].busy_block.busy_size == mdp2->heapinfo[block_pointed2].busy_block.busy_size){
                
                if(memcmp(addr_block_pointed1, addr_block_pointed2, mdp1->heapinfo[block_pointed1].busy_block.busy_size) != 0){
                  distance++;
                }else{
                  fprintf(stderr, "False difference detected\n");
                }
                
              }else{
                distance++;
              }
              
            }else{ // Fragmented block
              
              frag_pointed1 = ((char *)address_pointed1 - (char *)((void*) (((ADDR2UINT((size_t)block_pointed1)) - 1) * BLOCKSIZE + (char*)((struct mdesc*)s_heap)->heapbase))) / pow( 2, mdp1->heapinfo[block_pointed1].type);
              frag_pointed2 = ((char *)address_pointed2 - (char *)((void*) (((ADDR2UINT((size_t)block_pointed2)) - 1) * BLOCKSIZE + (char*)((struct mdesc*)s_heap)->heapbase))) / pow( 2, mdp1->heapinfo[block_pointed1].type);
              
              fprintf(stderr, "Fragments pointed : %d - %d\n", frag_pointed1, frag_pointed2);
              
              if((frag_pointed1 < 0) || (frag_pointed1 > (BLOCKSIZE / pow( 2, mdp1->heapinfo[block_pointed1].type))) || (frag_pointed2 < 0) || (frag_pointed2 > (BLOCKSIZE / pow( 2, mdp1->heapinfo[block_pointed1].type)))){
                fprintf(stderr, "Unknown pointer ! \n");
                fflush(NULL);
                distance++;
                continue;
              } 

              fprintf(stderr, "Size used in fragments pointed : %d - %d\n", mdp1->heapinfo[block_pointed1].busy_frag.frag_size[frag_pointed1], mdp2->heapinfo[block_pointed2].busy_frag.frag_size[frag_pointed2]);  
              
              if(mdp1->heapinfo[block_pointed1].busy_frag.frag_size[frag_pointed1] == mdp2->heapinfo[block_pointed2].busy_frag.frag_size[frag_pointed2]){
                
                if(memcmp(addr_block_pointed1, addr_block_pointed2, mdp1->heapinfo[block_pointed1].busy_frag.frag_size[frag_pointed1]) != 0){
                  distance++;
                }else{
                  fprintf(stderr, "False difference detected\n");
                }
                
              }else{
                distance ++;
              }
            }
            
          }else{
            fprintf(stderr, "Pointers on blocks with different types \n");
            distance++;
          }
        }
     
      }


      if(distance>0){
        fprintf(stderr,"\nDifferent data in large block %zu (size = %zu (in blocks), busy_size = %zu (in bytes))\n", i, mdp1->heapinfo[i].busy_block.size, mdp1->heapinfo[i].busy_block.busy_size);
        fflush(NULL);
        fprintf(stderr, "Hamming distance between blocks : %d\n", distance);
        mmalloc_backtrace_block_display(mdp1, i);
        mmalloc_backtrace_block_display(mdp2, i);
        fprintf(stderr, "\n");
        errors++;
        total_distance += distance;
      }

      i++;

    }else{

      if(mdp1->heapinfo[i].type > 0){ /* busy fragmented block */

        if(mdp1->heapinfo[i].type != mdp2->heapinfo[i].type){
          fprintf(stderr,"Different size of fragments in fragmented block %zu : %d - %d\n", i, mdp1->heapinfo[i].type, mdp2->heapinfo[i].type); fflush(NULL);
          errors++;
        }

        if(mdp1->heapinfo[i].busy_frag.nfree != mdp2->heapinfo[i].busy_frag.nfree){
          fprintf(stderr,"Different free fragments in fragmented block %zu : %zu - %zu\n", i, mdp1->heapinfo[i].busy_frag.nfree, mdp2->heapinfo[i].busy_frag.nfree); fflush(NULL);
          errors++;
        }

        if(mdp1->heapinfo[i].busy_frag.first != mdp2->heapinfo[i].busy_frag.first){
          fprintf(stderr,"Different busy_size of a large cluster : %zu - %zu\n", mdp1->heapinfo[i].busy_block.busy_size, mdp2->heapinfo[i].busy_block.busy_size); fflush(NULL);
          errors++;
        }

        frag_size = pow(2, mdp1->heapinfo[i].type);

        for(j=0; j< (BLOCKSIZE/frag_size); j++){

          if(mdp1->heapinfo[i].busy_frag.frag_size[j] != mdp2->heapinfo[i].busy_frag.frag_size[j]){
            fprintf(stderr,"Different busy_size for fragment %zu in block %zu : %hu - %hu\n", j, i, mdp1->heapinfo[i].busy_frag.frag_size[j], mdp2->heapinfo[i].busy_frag.frag_size[j]); fflush(NULL);
            errors++;
          }

          if(mdp1->heapinfo[i].busy_frag.frag_size[j] > 0){

            addr_frag1 = (char *)addr_block1 + (j * frag_size);
            xbt_assert(addr_frag1 < breakval1, "Fragment address out of heap memory used");

            addr_frag2 = (char *)addr_block2 + (j * frag_size);
            xbt_assert(addr_frag1 < breakval1, "Fragment address out of heap memory used");

            /* Hamming distance on different blocks */
            distance = 0;

            for(k=0;k<mdp1->heapinfo[i].busy_frag.frag_size[j];k++){

              if(memcmp(((char *)addr_frag1) + k, ((char *)addr_frag2) + k, 1) != 0){

                fprintf(stderr, "Different byte (offset=%d) (%p - %p) in fragment %zu in block %zu\n", k, (char *)addr_frag1 + k, (char *)addr_frag2 + k, j, i); fflush(NULL);

                pointer_align = (k / sizeof(void*)) * sizeof(void*);
                address_pointed1 = *((void **)((char *)addr_frag1 + pointer_align));
                address_pointed2 = *((void **)((char *)addr_frag2 + pointer_align));

                fprintf(stderr, "Addresses pointed : %p - %p\n", address_pointed1, address_pointed2);

                block_pointed1 = ((char*)address_pointed1 - (char*)((struct mdesc*)s_heap)->heapbase) / BLOCKSIZE + 1;
                block_pointed2 = ((char*)address_pointed2 - (char*)((struct mdesc*)s_heap)->heapbase) / BLOCKSIZE + 1;

                fprintf(stderr, "Blocks pointed : %d - %d\n", block_pointed1, block_pointed2);
                
                if((char *) address_pointed1 < (char*)((struct mdesc*)s_heap)->heapbase || block_pointed1 > mdp1->heapsize || block_pointed1 < 1 || (char *) address_pointed2 < (char*)((struct mdesc*)s_heap)->heapbase || block_pointed2 > mdp2->heapsize || block_pointed2 < 1) {
                  fprintf(stderr, "Unknown pointer ! \n");
                  fflush(NULL);
                  distance++;
                  continue;
                }

                addr_block_pointed1 = ((void*) (((ADDR2UINT((size_t)block_pointed1)) - 1) * BLOCKSIZE + (char*)heapbase1));
                addr_block_pointed2 = ((void*) (((ADDR2UINT((size_t)block_pointed2)) - 1) * BLOCKSIZE + (char*)heapbase2));
                
                if(mdp1->heapinfo[block_pointed1].type == mdp2->heapinfo[block_pointed2].type){
                  
                  if(mdp1->heapinfo[block_pointed1].type == 0){ // Large block
                    
                    if(mdp1->heapinfo[block_pointed1].busy_block.busy_size == mdp2->heapinfo[block_pointed2].busy_block.busy_size){
                      
                      if(memcmp(addr_block_pointed1, addr_block_pointed2, mdp1->heapinfo[block_pointed1].busy_block.busy_size) != 0){
                        distance++;
                      }else{
                        fprintf(stderr, "False difference detected\n");
                      }
                      
                    }else{
                      distance++;
                    }
                    
                  }else{ // Fragmented block

                    frag_pointed1 = ((char *)address_pointed1 - (char *)((void*) (((ADDR2UINT((size_t)block_pointed1)) - 1) * BLOCKSIZE + (char*)((struct mdesc*)s_heap)->heapbase))) / pow( 2, mdp1->heapinfo[block_pointed1].type);
                    frag_pointed2 = ((char *)address_pointed2 - (char *)((void*) (((ADDR2UINT((size_t)block_pointed2)) - 1) * BLOCKSIZE + (char*)((struct mdesc*)s_heap)->heapbase))) / pow( 2, mdp1->heapinfo[block_pointed1].type);

                    fprintf(stderr, "Fragments pointed : %d - %d\n", frag_pointed1, frag_pointed2);
                    
                    if((frag_pointed1 < 0) || (frag_pointed1 > (BLOCKSIZE / pow( 2, mdp1->heapinfo[block_pointed1].type))) || (frag_pointed2 < 0) || (frag_pointed2 > (BLOCKSIZE / pow( 2, mdp1->heapinfo[block_pointed1].type)))){
                       fprintf(stderr, "Unknown pointer ! \n");
                       fflush(NULL);
                       distance++;
                       continue;
                    } 

                    fprintf(stderr, "Size used in fragments pointed : %d - %d\n", mdp1->heapinfo[block_pointed1].busy_frag.frag_size[frag_pointed1], mdp2->heapinfo[block_pointed2].busy_frag.frag_size[frag_pointed2]); 
                                        
                    if(mdp1->heapinfo[block_pointed1].busy_frag.frag_size[frag_pointed1] == mdp2->heapinfo[block_pointed2].busy_frag.frag_size[frag_pointed2]){
                      
                      if(memcmp(addr_block_pointed1, addr_block_pointed2, mdp1->heapinfo[block_pointed1].busy_frag.frag_size[frag_pointed1]) != 0){
                        distance++;
                      }else{
                        fprintf(stderr, "False difference detected\n");
                      }
                      
                    }else{
                      distance ++;
                    }
                  }

                }else{
                  fprintf(stderr, "Pointers on blocks with different types \n");
                  distance++;
                }
              }

            }

            if(distance > 0){
              fprintf(stderr,"\nDifferent data in fragment %zu (size = %zu, size used = %hu) in block %zu \n", j, frag_size, mdp1->heapinfo[i].busy_frag.frag_size[j], i);
              fprintf(stderr, "Hamming distance between fragments : %d\n", distance);
              mmalloc_backtrace_fragment_display(mdp1, i, j);
              mmalloc_backtrace_fragment_display(mdp2, i, j);
              fprintf(stderr, "\n");
              errors++;
              total_distance += distance;

            }

          }
        }

        i++;

      }else{ /* free block */

        i++;

      }

    }

  }


  fprintf(stderr, "Hamming distance between heap regions : %d\n", total_distance);

  return (errors);
}


/* void *get_end_addr_heap(void *heap){ */

/*   FILE *fp;                     /\* File pointer to process's proc maps file *\/ */
/*   char *line = NULL;            /\* Temporal storage for each line that is readed *\/ */
/*   ssize_t read;                 /\* Number of bytes readed *\/ */
/*   size_t n = 0;                 /\* Amount of bytes to read by getline *\/ */

/*   fp = fopen("/proc/self/maps", "r"); */

/*   if(fp == NULL) */
/*     perror("fopen failed"); */


/*   xbt_dynar_t lfields = NULL; */
/*   xbt_dynar_t start_end  = NULL; */
/*   void *start_addr; */
/*   void *end_addr; */

/*   while ((read = getline(&line, &n, fp)) != -1) { */

/*     xbt_str_trim(line, NULL); */
/*     xbt_str_strip_spaces(line); */
/*     lfields = xbt_str_split(line,NULL); */

/*     start_end = xbt_str_split(xbt_dynar_get_as(lfields, 0, char*), "-"); */
/*     start_addr = (void *) strtoul(xbt_dynar_get_as(start_end, 0, char*), NULL, 16); */
/*     end_addr = (void *) strtoul(xbt_dynar_get_as(start_end, 1, char*), NULL, 16); */

/*     if(start_addr == heap){ */
/*       free(line); */
/*       fclose(fp); */
/*       xbt_dynar_reset(lfields); */
/*       xbt_free(lfields); */
/*       xbt_dynar_reset(start_end); */
/*       xbt_free(start_end); */
/*       return end_addr; */
/*     } */

/*   } */

/*   xbt_dynar_reset(lfields); */
/*   xbt_free(lfields); */
/*   xbt_dynar_reset(start_end); */
/*   xbt_free(start_end); */
/*   free(line); */
/*   fclose(fp); */
/*   return NULL; */


/* } */


void mmalloc_display_info_heap(xbt_mheap_t h){

}



