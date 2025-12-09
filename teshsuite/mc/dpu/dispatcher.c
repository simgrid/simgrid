// Copyright (C) 2010-2017  Cesar Rodriguez <cesar.rodriguez@lipn.fr>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.

#include <pthread.h>
#include <stdio.h>
#include <assert.h>

/*
 This benchmark simulates communication between
 one application and a set of servers over a 
 lossy communication channel.

 This is simulated as follows: 
 The main function sends requests via a channel to one server
 which is represented as an element of an array.

 Either because the server is busy or because 
 the channel is lossy, the request might be lost.
 This is simulated by the fact that the server
 just checks if there is something in the input
 channel and terminates.
 
 This server is chosen non-deterministically
 and the main function checks in the end 
 that all servers chosen were valid and how 
 many requests were lost.
 There is a non-trivial assertion because 
 two adjacent packages cannot be sent to the
 same server, thus a server can at most 
 receive NUM_REQUESTS/2 + 1 if the number 
 of servers is greater.

 For a fixed RNUM (e.g. RNUM 1) and an 
 increasing SNUM, we obtain an exponential
 number of SSBs in nidhugg.
 
 For a fixed SNUM > 1 (e.g. SNUM 2) and
 an increasing RNUM, we obtain a linear
 number of SSBs in nidhugg. 
*/

#ifndef PARAM1
#define PARAM1 5
#endif

#ifndef PARAM2
#define PARAM2 2
#endif

#define SNUM PARAM1  // number of servers
#define RNUM PARAM2  // number of requests

pthread_mutex_t servers[SNUM];
pthread_mutex_t k;
int sid = 0;           // server id
int in_channel[SNUM];  // input  communication channel
int out_channel[SNUM]; // output communicatino channel

// this thread just chooses a server
// different from the last one if there 
// are more servers than requests
// Right now, it just chooses a server
static void *pick_server(void *arg)
{
 //unsigned last_server_id = (unsigned long) arg;

 // printf ("pick_server: last sid %d\n", last_server_id); 
 
 for (int x = 0; x < SNUM; x++)
 {
 //  if (x == last_server_id || x == sid)
 //    continue;
   //printf ("pick_server: before lock k\n");
   pthread_mutex_lock(&k);
   //printf ("pick_server: after lock k\n");
    sid = x; 
 //   printf ("pick_server: sid %d\n", sid);
   pthread_mutex_unlock(&k);
   //printf ("pick_server: after unlock k\n");
 }
 //printf ("pick_server: exited\n");
 return 0;
}

// this thread represents a server 
static void *server(void *arg)
{
 unsigned id = (unsigned long) arg;

 // locks on the server mutex
 pthread_mutex_lock(&servers[id]);
  //printf ("server: lock servers %u\n",id);
  //  printf ("server %u: works\n", id);
  // checks if it has something in 
  // the input channel and if so 
  // signals that to the out channel.
  if (in_channel[id] > 0)
    out_channel[id] = in_channel[id]; 
 pthread_mutex_unlock(&servers[id]);
 //printf ("server: unlock servers %u\n",id);

 return 0;
}

// this function sends requests to a server
static int process_request(int req_id, int last_server_id)
{
 pthread_t s;
 int idx = 0;

 // picks a server non-deterministically
 pthread_create(&s, NULL, pick_server, (void*) (long) last_server_id);

 //printf ("process_request %d: after create s\n",req_id);
 // get the server chosen
 pthread_mutex_lock (&k);
 //printf ("process_request %d: lock k\n",req_id);
  idx = sid;
  // printf ("process_request: read choice from pick_server %d\n", idx);
  // if (idx == last_server_id && SNUM > 0) 
  // {
  //   idx = idx / SNUM;
  // }
  // printf ("process_request: chosen server %d\n", idx);
 pthread_mutex_unlock (&k);
 //printf ("process_request %d: unlock k\n",req_id);

 if (SNUM > 0)
 {
  // send req to server 
  pthread_mutex_lock (&servers[idx]);
   //printf ("process_request %d: lock servers %d\n",req_id,idx);
   // printf ("process_request: req %d sent to server %d\n", req_id, idx);
   in_channel[idx] = in_channel[idx] + 1;
  pthread_mutex_unlock (&servers[idx]);
  //printf ("process_request %d: unlock servers %d\n",req_id,idx);
 }

 //printf ("process_request %d: before joining s\n",req_id);
 pthread_join(s, NULL);
 //printf ("process_request %d: after joining s\n",req_id);

 return idx;
}

int main()
{
  pthread_t idw[SNUM];
  int picked_servers[RNUM];

  //printf ("== start ==\n");

  // spawn SNUM servers 
  pthread_mutex_init(&k, NULL);
  for (int x = 0; x < SNUM; x++)
  {
    in_channel[x] = 0;
    out_channel[x] = 0;
    pthread_mutex_init(&servers[x], NULL);
    pthread_create(&idw[x], 0, server, (void*) (long) x);
  }

  // process requests
  for (int r = 0; r < RNUM; r++){
    if (r == 0)
      picked_servers[r] = process_request(r, SNUM);
    else
      picked_servers[r] = process_request(r,picked_servers[r-1]); 
  }
  
  pthread_exit (0);

#if 0
  //printf ("main: before joining servers\n");
  // wait for servers to join
  for (int x = 0; x < SNUM; x++)
    pthread_join(idw[x],NULL);

  // process results:
  // - counts the number of requests that failed 
  // - assert that the picked server was good 
  // - assert that a server could only be
  int missed_req = 0;

  for (int r = 0; r < RNUM; r++)
  {
    int s = picked_servers[r];
    // printf ("main: s %d, val %d\n", s, out_channel[s]);
    // bound check
    if (SNUM > 0)
      assert(s >= 0 && s < SNUM);
    // value in the out_channel
    //if (SNUM > RNUM)
    //  assert(out_channel[s] <= RNUM/2 + 1);
    //else
    //  assert(out_channel[s] <= RNUM);
    // if the value is 0 it means it was a miss
    if (out_channel[s])
      out_channel[s] = out_channel[s] - 1;
    else
      missed_req++;
  }
  
  // printf ("total number of missed %d\n", missed_req);
  //printf ("== end ==\n");
  return 0;
#endif
}
