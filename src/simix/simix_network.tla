---- MODULE simix_network ----
(* Copyright (c) 2012-2021. The SimGrid Team. All rights reserved.          *)

(* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. *)

(* This is a TLA module specifying the networking layer of SIMIX.
   It is used to verify the soundness of the DPOR reduction algorithm
   used in the model-checker.

   If you think you found a new independence lemma, add it to this
   file and relaunch TLC to check whether your lemma actually holds.
   *)
EXTENDS Naturals, Sequences, FiniteSets
CONSTANTS RdV, Addr, Proc, ValTrue, ValFalse, SendIns, RecvIns, WaitIns,
          TestIns, LocalIns
VARIABLES network, memory, pc

NoProc == CHOOSE p : p \notin Proc
NoAddr == CHOOSE a : a \notin Addr

Partition(S) == \forall x,y \in S : x \cap y /= {} => x = y

Comm == [id:Nat,
         rdv:RdV,
         status:{"send","recv","ready","done"},
         src:Proc,
         dst:Proc,
         data_src:Addr,
         data_dst:Addr]

ASSUME ValTrue \in Nat
ASSUME ValFalse \in Nat

(* The set of all the instructions *)
ASSUME Partition({SendIns, RecvIns, WaitIns, TestIns, LocalIns})
Instr == UNION {SendIns, RecvIns, WaitIns, TestIns, LocalIns}

------------------------------------------
(* Independence operator *)
I(A,B) == ENABLED A /\ ENABLED B => /\ A => (ENABLED B)'
                                    /\ B => (ENABLED A)'
                                    /\ A \cdot B \equiv B \cdot A

(* Initially there are no messages in the network and the memory can have anything in their memories *)
Init == /\ network = {}
        /\ memory \in [Proc -> [Addr -> Nat]]
        /\ pc = CHOOSE f : f \in [Proc -> Instr]

(* Let's keep everything in the right domains *)
TypeInv == /\ network \subseteq Comm
           /\ memory \in [Proc -> [Addr -> Nat]]
           /\ pc \in [Proc -> Instr]

(* The set of all communications waiting at rdv *)
mailbox(rdv) == {comm \in network : comm.rdv=rdv /\ comm.status \in {"send","recv"}}

(* The set of memory addresses of a process being used in a communication *)
CommBuffers(pid) ==
  {c.data_src: c \in { y \in network: y.status /= "done" /\ (y.src = pid \/ y.dst = pid)}}
\cup {c.data_dst: c \in { y \in network: y.status /= "done" /\ (y.src = pid \/ y.dst = pid)}}

(* This is a send step of the system *)
(* pid: the process ID of the sender *)
(* rdv: the rendez-vous point where the "send" communication request is going to be pushed *)
(* data_r: the address in the sender's memory where the data is stored *)
(* comm_r: the address in the sender's memory where to store the communication id *)
Send(pid, rdv, data_r, comm_r) ==
  /\ rdv \in RdV
  /\ pid \in Proc
  /\ data_r \in Addr
  /\ comm_r \in Addr
  /\ pc[pid] \in SendIns

     (* A matching recv request exists in the rendez-vous *)
     (* Complete the sender fields and set the communication to the ready state *)
  /\ \/ \exists c \in mailbox(rdv):
          /\ c.status="recv"
          /\ \forall d \in mailbox(rdv): d.status="recv" => c.id <= d.id
          /\ network' =
               (network \ {c}) \cup {[c EXCEPT
                                       !.status = "ready",
                                       !.src = pid,
                                       !.data_src = data_r]}
          (* Use c's existing communication id *)
          /\ memory' = [memory EXCEPT ![pid][comm_r] = c.id]


     (* No matching recv communication request exists. *)
     (* Create a send request and push it in the network. *)
     \/ /\ ~ \exists c \in mailbox(rdv): c.status = "recv"
        /\ LET comm ==
                 [id |-> Cardinality(network)+1,
                  rdv |-> rdv,
                  status |-> "send",
                  src |-> pid,
                  dst |-> NoProc,
                  data_src |-> data_r,
                  data_dst |-> NoAddr]
           IN
             /\ network' = network \cup {comm}
             /\ memory' = [memory EXCEPT ![pid][comm_r] = comm.id]
  /\ \E ins \in Instr : pc' = [pc EXCEPT ![pid] = ins]

(* This is a receive step of the system *)
(* pid: the process ID of the receiver *)
(* rdv: the Rendez-vous where the "receive" communication request is going to be pushed *)
(* data_r: the address in the receivers's memory where the data is going to be stored *)
(* comm_r: the address in the receivers's memory where to store the communication id *)
Recv(pid, rdv, data_r, comm_r) ==
  /\ rdv \in RdV
  /\ pid \in Proc
  /\ data_r \in Addr
  /\ comm_r \in Addr
  /\ pc[pid] \in RecvIns

     (* A matching send request exists in the rendez-vous *)
     (* Complete the receiver fields and set the communication to the ready state *)
  /\ \/ \exists c \in mailbox(rdv):
          /\ c.status="send"
          /\ \forall d \in mailbox(rdv): d.status="send" => c.id <= d.id
          /\ network' =
               (network \ {c}) \cup {[c EXCEPT
                                       !.status = "ready",
                                       !.dst = pid,
                                       !.data_dst = data_r]}
          (* Use c's existing communication id *)
          /\ memory' = [memory EXCEPT ![pid][comm_r] = c.id]


     (* No matching send communication request exists. *)
     (* Create a recv request and push it in the network. *)
     \/ /\ ~ \exists c \in mailbox(rdv): c.status = "send"
        /\ LET comm ==
                 [id |-> Cardinality(network)+1,
                  status |-> "recv",
                  dst |-> pid,
                  data_dst |-> data_r]
           IN
             /\ network' = network \cup {comm}
             /\ memory' = [memory EXCEPT ![pid][comm_r] = comm.id]
  /\ \E ins \in Instr : pc' = [pc EXCEPT ![pid] = ins]

(* Wait for at least one communication from a given list to complete *)
(* pid: the process ID issuing the wait *)
(* comms: the list of addresses in the process's memory where the communication ids are stored *)
Wait(pid, comms) ==
  /\ comms \subseteq Addr
  /\ pid \in Proc
  /\ pc[pid] \in WaitIns
  /\ \E comm_r \in comms, c \in network: c.id = memory[pid][comm_r] /\
     \/ /\ c.status = "ready"
        /\ memory' = [memory EXCEPT ![c.dst][c.data_dst] = memory[c.src][c.data_src]]
        /\ network' = (network \ {c}) \cup {[c EXCEPT !.status = "done"]}
     \/ /\ c.status = "done"
        /\ UNCHANGED <<memory,network>>
  /\ \E ins \in Instr : pc' = [pc EXCEPT ![pid] = ins]

(* Test if at least one communication from a given list has completed *)
(* pid: the process ID issuing the wait *)
(* comms: the list of addresses in the process's memory where the communication ids are stored *)
(* ret_r: the address in the process's memory where the result is going to be stored *)
Test(pid, comms, ret_r) ==
  /\ comms \subseteq Addr
  /\ ret_r \in Addr
  /\ pid \in Proc
  /\ pc[pid] \in TestIns
  /\ \/ \E comm_r \in comms, c\in network: c.id = memory[pid][comm_r] /\
        \/ /\ c.status = "ready"
           /\ memory' = [memory EXCEPT ![c.dst][c.data_dst] = memory[c.src][c.data_src],
                                        ![pid][ret_r] = ValTrue]
           /\ network' = (network \ {c}) \cup {[c EXCEPT !.status = "done"]}
        \/ /\ c.status = "done"
           /\ memory' = [memory EXCEPT ![pid][ret_r] = ValTrue]
           /\ UNCHANGED network
     \/ ~ \exists comm_r \in comms, c \in network: c.id = memory[pid][comm_r]
        /\ c.status \in {"ready","done"}
        /\ memory' = [memory EXCEPT ![pid][ret_r] = ValFalse]
        /\ UNCHANGED network
  /\ \E ins \in Instr : pc' = [pc EXCEPT ![pid] = ins]

(* Local instruction execution *)
Local(pid) ==
    /\ pid \in Proc
    /\ pc[pid] \in LocalIns
    /\ memory' \in [Proc -> [Addr -> Nat]]
    /\ \forall p \in Proc, a \in Addr: memory'[p][a] /= memory[p][a]
       => p = pid /\ a \notin CommBuffers(pid)
    /\ \E ins \in Instr : pc' = [pc EXCEPT ![pid] = ins]
    /\ UNCHANGED network

Next == \exists p \in Proc, data_r \in Addr, comm_r \in Addr, rdv \in RdV,
                ret_r \in Addr, ids \in SUBSET network:
          \/ Send(p, rdv, data_r, comm_r)
          \/ Recv(p, rdv, data_r, comm_r)
          \/ Wait(p, comm_r)
          \/ Test(p, comm_r, ret_r)
          \/ Local(p)

Spec == Init /\ [][Next]_<<network,memory>>
-------------------------------
(* Independence of iSend / iRecv steps *)
THEOREM \forall p1, p2 \in Proc: \forall rdv1, rdv2 \in RdV:
        \forall data1, data2, comm1, comm2 \in Addr:
        /\ p1 /= p2
        /\ ENABLED Send(p1, rdv1, data1, comm1)
        /\ ENABLED Recv(p2, rdv2, data2, comm2)
        => I(Send(p1, rdv1, data1, comm1), Recv(p2, rdv2, data2, comm2))

(* Independence of iSend and Wait *)
THEOREM \forall p1, p2 \in Proc: \forall data, comm1, comm2 \in Addr:
        \forall rdv \in RdV: \exists c \in network:
        /\ p1 /= p2
        /\ c.id = memory[p2][comm2]
        /\ \/ (p1 /= c.dst /\ p1 /= c.src)
           \/ (comm1 /= c.data_src /\ comm1 /= c.data_dst)
        /\ ENABLED Send(p1, rdv, data, comm1)
        /\ ENABLED Wait(p2, comm2)
        => I(Send(p1, rdv, data, comm1), Wait(p2, comm2))

(* Independence of iSend's in different rendez-vous *)
THEOREM \forall p1, p2 \in Proc: \forall rdv1, rdv2 \in RdV:
        \forall data1, data2, comm1, comm2 \in Addr:
        /\ p1 /= p2
        /\ rdv1 /= rdv2
        /\ ENABLED Send(p1, rdv1, data1, comm1)
        /\ ENABLED Send(p2, rdv2, data2, comm2)
        => I(Send(p1, rdv1, data1, comm1),
             Send(p2, rdv2, data2, comm2))

(* Independence of iRecv's in different rendez-vous *)
THEOREM \forall p1, p2 \in Proc: \forall rdv1, rdv2 \in RdV:
        \forall data1, data2, comm1, comm2 \in Addr:
        /\ p1 /= p2
        /\ rdv1 /= rdv2
        /\ ENABLED Recv(p1, rdv1, data1, comm1)
        /\ ENABLED Recv(p2, rdv2, data2, comm2)
        => I(Recv(p1, rdv1, data1, comm1),
             Recv(p2, rdv2, data2, comm2))

(* Independence of Wait of different processes on the same comm *)
THEOREM \forall p1, p2 \in Proc: \forall comm1, comm2 \in Addr:
        /\ p1 /= p2
        /\ comm1 = comm2
        /\ ENABLED Wait(p1, comm1)
        /\ ENABLED Wait(p2, comm2)
        => I(Wait(p1, comm1), Wait(p2, comm2))
====
\* Generated at Thu Feb 18 13:49:35 CET 2010
