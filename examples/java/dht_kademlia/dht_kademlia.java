/* Copyright (c) 2012-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;
import org.simgrid.s4u.*;

/* ------------------------------------------------------------------------ */
/* Constants shared by all files.                                           */
/*                                                                          */
/* Node identifiers are represented as Java `long`, even though they only   */
/* use the low 32 bits: this avoids all the sign-related pitfalls of        */
/* mapping C++'s `unsigned int` onto Java's (signed) `int`.                 */
/* ------------------------------------------------------------------------ */

public class dht_kademlia {
  static final double FIND_NODE_TIMEOUT        = 10.0;
  static final double FIND_NODE_GLOBAL_TIMEOUT = 50.0;

  static final int KADEMLIA_ALPHA = 3;
  static final int BUCKET_SIZE    = 20;

  static final int IDENTIFIER_SIZE = 32;

  static final double RANDOM_LOOKUP_INTERVAL = 100.0;

  static final int MAX_STEPS = 10;

  static final int JOIN_BUCKETS_QUERIES = 5;

  static final long RANDOM_LOOKUP_NODE = 0;

  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    /* Check the arguments */
    if (args.length < 2)
      Engine.die("Usage: dht_kademlia platform_file deployment_file\n"
                 + "\tExample: cluster_backbone.xml dht_kademlia_d.xml\n");

    e.load_platform(args[0]);
    e.load_deployment(args[1]);

    e.run();

    Engine.info("Simulated time: %g", Engine.get_clock());

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}

/* ------------------------------------------------------------------------ */
/* The routing table                                                        */
/* ------------------------------------------------------------------------ */

/** Routing table bucket */
class Bucket {
  final long id;                          // bucket id
  Deque<Long> nodes = new ArrayDeque<>(); // Nodes in the bucket.

  Bucket(long id) { this.id = id; }
  long getId() { return id; }
}

/** Node routing table */
class RoutingTable {
  long id;                                  // node id of the client's routing table
  List<Bucket> buckets = new ArrayList<>(); // Node bucket list

  RoutingTable(long node_id)
  {
    id = node_id;
    for (long i = 0; i < dht_kademlia.IDENTIFIER_SIZE + 1; i++)
      buckets.add(new Bucket(i));
  }

  void print()
  {
    Engine.info("Routing table of %08x:", id);
    for (int i = 0; i <= dht_kademlia.IDENTIFIER_SIZE; i++) {
      if (!buckets.get(i).nodes.isEmpty()) {
        Engine.info("Bucket number %d: ", i);
        int j = 0;
        for (long value : buckets.get(i).nodes) {
          Engine.info("Element %d: %08x", j, value);
          j++;
        }
      }
    }
  }

  /** Finds the corresponding bucket in a routing table for a given identifier */
  Bucket findBucket(long id)
  {
    long xor_number = this.id ^ id;
    int prefix      = get_node_prefix(xor_number, dht_kademlia.IDENTIFIER_SIZE);
    if (prefix > dht_kademlia.IDENTIFIER_SIZE)
      Engine.die("Tried to return a bucket that doesn't exist.");
    return buckets.get(prefix);
  }

  /** Returns if the routing table contains the id. */
  boolean contains(long node_id) { return findBucket(node_id).nodes.contains(node_id); }

  /**
   * Returns an identifier which is in a specific bucket of a routing table
   * @param id id of the routing table owner
   * @param prefix id of the bucket where we want that identifier to be
   */
  static long get_id_in_prefix(long id, int prefix)
  {
    if (prefix == 0)
      return 0;
    return (1L << (prefix - 1)) ^ id;
  }

  /**
   * Returns the prefix of an identifier.
   * The prefix is the id of the bucket in which the remote identifier xor our identifier should be stored.
   * @param id big (32 bits, stored in a long) id to test
   * @param nb_bits key size
   */
  static int get_node_prefix(long id, int nb_bits)
  {
    int size = 32;
    for (int j = 0; j < size; j++)
      if (((id >> (size - 1 - j)) & 0x1) != 0)
        return nb_bits - j;
    return 0;
  }
}

/* ------------------------------------------------------------------------ */
/* Answer                                                                   */
/* ------------------------------------------------------------------------ */

/**
 * A (contact, distance) pair, as found in a routing table bucket. Two contacts are considered equal
 *  as soon as they share the same id, regardless of their (destination-dependent) distance.
 */
class Contact {
  long id;
  long distance;
  Contact(long id, long distance)
  {
    this.id       = id;
    this.distance = distance;
  }
  @Override public boolean equals(Object o) { return (o instanceof Contact) && ((Contact)o).id == id; }
  @Override public int hashCode() { return Long.hashCode(id); }
}

/** Node query answer. contains the elements closest to the id given. */
class Answer {
  long destination_id;
  List<Contact> nodes = new ArrayList<>();

  Answer(long destination_id) { this.destination_id = destination_id; }
  long getDestinationId() { return destination_id; }
  int getSize() { return nodes.size(); }
  List<Contact> getNodes() { return nodes; }

  /** Prints an Answer, for debugging purposes */
  void print()
  {
    Engine.info("Searching %08x, size %d", destination_id, nodes.size());
    int i = 0;
    for (Contact contact : nodes)
      Engine.info("Node %08x: %08x is at distance %d", i++, contact.id, contact.distance);
  }

  /**
   * Merge two answers together, only keeping the best nodes
   * @param source the source of the nodes to add
   */
  int merge(Answer source)
  {
    if (this == source)
      return 0;

    int nb_added = 0;
    for (Contact contact : source.nodes) {
      if (!nodes.contains(contact)) {
        nodes.add(contact);
        nb_added++;
      }
    }
    trim();
    return nb_added;
  }

  /** Trims an Answer, in order for it to have a size of less or equal to "BUCKET_SIZE" */
  void trim()
  {
    nodes.sort((a, b) -> Long.compare(a.distance, b.distance));
    if (nodes.size() > dht_kademlia.BUCKET_SIZE)
      nodes.subList(dht_kademlia.BUCKET_SIZE, nodes.size()).clear();
  }

  /** Returns if the destination we are trying to find is found */
  boolean destinationFound() { return !nodes.isEmpty() && nodes.get(0).distance == 0; }

  /** Adds the content of a bucket into an answer object. */
  void addBucket(Bucket bucket)
  {
    if (bucket == null)
      Engine.die("Provided a null bucket");
    for (long id : bucket.nodes) {
      long distance = id ^ destination_id;
      nodes.add(new Contact(id, distance));
    }
  }
}

/* ------------------------------------------------------------------------ */
/* Message                                                                  */
/* ------------------------------------------------------------------------ */

class Message {
  long sender_id;          // Id of the guy who sent the message
  long destination_id;     // Id we are trying to find, if needed.
  Answer answer;           // Answer to the request made, if needed.
  Mailbox answer_to;       // mailbox to send the answer to (if not an answer).
  String issuer_host_name; // used for logging

  Message(long sender_id, long destination_id, Answer answer, Mailbox mailbox, String hostname)
  {
    this.sender_id        = sender_id;
    this.destination_id   = destination_id;
    this.answer           = answer;
    this.answer_to        = mailbox;
    this.issuer_host_name = hostname;
  }
  Message(long sender_id, long destination_id, Mailbox mailbox, String hostname)
  {
    this(sender_id, destination_id, null, mailbox, hostname);
  }
}

/* ------------------------------------------------------------------------ */
/* class Node                                                               */
/*                                                                          */
/* This plain object holds the Kademlia routing logic. It is not an Actor   */
/* by itself: the actor entry point is the  NodeActor class below, which    */
/* owns and drives a Node instance.                                         */
/* ------------------------------------------------------------------------ */

class Node {
  long id;                   // node id - 160 bits in the original protocol, 32 here
  RoutingTable table;        // node routing table
  int find_node_success = 0; // Number of find_node which have succeeded.
  int find_node_failed  = 0; // Number of find_node which have failed.
  Comm receive_comm     = null;
  Message received_msg  = null;

  NodeActor actor; // the actor driving this node, used to reach the engine, its mailboxes, and sleep

  Node(long node_id, NodeActor actor)
  {
    id         = node_id;
    table      = new RoutingTable(node_id);
    this.actor = actor;
  }

  long getId() { return id; }

  /** Try to asynchronously get a new message from given mailbox. Return null if none available. */
  Message receive(Mailbox mailbox)
  {
    if (receive_comm == null)
      receive_comm = mailbox.get_async();
    if (!receive_comm.test())
      return null;
    received_msg = (Message)receive_comm.get_payload();
    receive_comm = null;
    return received_msg;
  }

  /**
   * Tries to join the network
   * @param known_id id of the node I know in the network.
   */
  boolean join(long known_id)
  {
    boolean got_answer = false;

    /* Add the guy we know to our routing table and ourselves. */
    routingTableUpdate(id);
    routingTableUpdate(known_id);

    /* First step: Send a "FIND_NODE" request to the node we know */
    sendFindNode(known_id, id);

    Mailbox mailbox = actor.get_engine().mailbox_by_name(String.valueOf(id));
    do {
      Message msg = receive(mailbox);
      if (msg != null) {
        Engine.debug("Received an answer from the node I know.");
        got_answer = true;
        // retrieve the node list and ping them.
        if (msg.answer != null) {
          for (Contact contact : msg.answer.getNodes())
            routingTableUpdate(contact.id);
        } else {
          handleFindNode(msg);
        }
      } else {
        actor.sleepFor(1);
      }
    } while (!got_answer);

    /* Second step: Send a FIND_NODE to a random node in buckets */
    long bucket_id = table.findBucket(known_id).getId();
    for (long i = 0;
         ((bucket_id > i) || (bucket_id + i) <= dht_kademlia.IDENTIFIER_SIZE) && i < dht_kademlia.JOIN_BUCKETS_QUERIES;
         i++) {
      if (bucket_id > i) {
        long id_in_bucket = RoutingTable.get_id_in_prefix(id, (int)(bucket_id - i));
        findNode(id_in_bucket, false);
      }
      if (bucket_id + i <= dht_kademlia.IDENTIFIER_SIZE) {
        long id_in_bucket = RoutingTable.get_id_in_prefix(id, (int)(bucket_id + i));
        findNode(id_in_bucket, false);
      }
    }
    return got_answer;
  }

  /**
   * Send a "FIND_NODE" to a node
   * @param id node we are querying
   * @param destination node we are trying to find.
   */
  void sendFindNode(long id, long destination)
  {
    /* Gets the mailbox to send to */
    Mailbox mailbox = actor.get_engine().mailbox_by_name(String.valueOf(id));
    /* Build the message */
    Message msg = new Message(this.id, destination, actor.get_engine().mailbox_by_name(String.valueOf(this.id)),
                              actor.get_host().get_name());

    /* Send the message */
    mailbox.put_init(msg, 1).detach();
    Engine.verbose("Asking %d for its closest nodes", id);
  }

  /**
   * Sends to the best "KADEMLIA_ALPHA" nodes in the "node_list" array a "FIND_NODE" request, to ask them for their
   *  best nodes
   */
  int sendFindNodeToBest(Answer node_list)
  {
    int i            = 0;
    int j            = 0;
    long destination = node_list.getDestinationId();
    for (Contact contact : node_list.getNodes()) {
      /* We need to have at most "KADEMLIA_ALPHA" requests each time, according to the protocol */
      if (contact.id != id) { /* No need to query ourselves */
        sendFindNode(contact.id, destination);
        j++;
      }
      i++;
      if (j == dht_kademlia.KADEMLIA_ALPHA)
        break;
    }
    return i;
  }

  /**
   * Updates/Puts the node id into our routing table
   * @param id The id of the node we need to add into our routing table
   */
  void routingTableUpdate(long id)
  {
    Bucket bucket = table.findBucket(id);

    if (!bucket.nodes.contains(id)) {
      /* We check if the bucket is full or not. If it is, we evict an old element */
      if (bucket.nodes.size() >= dht_kademlia.BUCKET_SIZE)
        bucket.nodes.removeLast();
      bucket.nodes.addFirst(id);
      Engine.verbose("I'm adding to my routing table %08x", id);
    } else {
      // We push the element to the front
      bucket.nodes.remove(id);
      bucket.nodes.addFirst(id);
      Engine.verbose("I'm updating %08x", id);
    }
  }

  /** Finds the closest nodes to the given destination id. */
  Answer findClosest(long destination_id)
  {
    Answer answer = new Answer(destination_id);
    /* We find the corresponding bucket for the id */
    Bucket bucket  = table.findBucket(destination_id);
    long bucket_id = bucket.getId();
    if (bucket_id > dht_kademlia.IDENTIFIER_SIZE)
      Engine.die("Bucket found has a wrong identifier");
    /* So, we copy the contents of the bucket into our answer */
    answer.addBucket(bucket);

    /* However, if we don't have enough elements in our bucket, we NEED to include at least "BUCKET_SIZE" elements
     * (if, of course, we know at least "BUCKET_SIZE" elements. So we're going to look into the other buckets. */
    for (int i = 1; answer.getSize() < dht_kademlia.BUCKET_SIZE &&
                    ((bucket_id - i > 0) || (bucket_id + i < dht_kademlia.IDENTIFIER_SIZE));
         i++) {
      /* We check the previous buckets */
      if (bucket_id - i >= 0)
        answer.addBucket(table.buckets.get((int)(bucket_id - i)));
      /* We check the next buckets */
      if (bucket_id + i <= dht_kademlia.IDENTIFIER_SIZE)
        answer.addBucket(table.buckets.get((int)(bucket_id + i)));
    }
    /* We trim the array to have only BUCKET_SIZE or less elements */
    answer.trim();

    return answer;
  }

  /** Send a request to find a node in the node routing table. */
  boolean findNode(long id_to_find, boolean count_in_stats)
  {
    int queries;
    int answers;
    boolean destination_found = false;
    int nodes_added           = 0;
    double global_timeout     = Engine.get_clock() + dht_kademlia.FIND_NODE_GLOBAL_TIMEOUT;
    int steps                 = 0;

    /* First we build a list of who we already know */
    Answer node_list = findClosest(id_to_find);
    Engine.debug("Doing a FIND_NODE on %08x", id_to_find);

    /* Ask the nodes on our list if they have information about the node we are trying to find */
    do {
      answers        = 0;
      queries        = sendFindNodeToBest(node_list);
      nodes_added    = 0;
      double timeout = Engine.get_clock() + dht_kademlia.FIND_NODE_TIMEOUT;
      steps++;
      double time_beginreceive = Engine.get_clock();

      Mailbox mailbox = actor.get_engine().mailbox_by_name(String.valueOf(id));
      do {
        Message msg = receive(mailbox);
        if (msg != null) {
          // Check if what we have received is what we are looking for.
          if (msg.answer != null && msg.answer.getDestinationId() == id_to_find) {
            routingTableUpdate(msg.sender_id);
            // Handle the answer
            for (Contact contact : node_list.getNodes())
              routingTableUpdate(contact.id);
            answers++;

            nodes_added = node_list.merge(msg.answer);
            Engine.debug("Received an answer from %s (%s) with %d nodes on it", msg.answer_to.get_name(),
                         msg.issuer_host_name, msg.answer.getSize());
          } else {
            if (msg.answer != null) {
              routingTableUpdate(msg.sender_id);
              Engine.debug("Received a wrong answer for a FIND_NODE");
            } else {
              handleFindNode(msg);
            }
            // Update the timeout if we didn't have our answer
            timeout += Engine.get_clock() - time_beginreceive;
            time_beginreceive = Engine.get_clock();
          }
        } else {
          actor.sleepFor(1);
        }
      } while (Engine.get_clock() < timeout && answers < queries);
      destination_found = node_list.destinationFound();
    } while (!destination_found && (nodes_added > 0 || answers == 0) && Engine.get_clock() < global_timeout &&
             steps < dht_kademlia.MAX_STEPS);

    if (destination_found) {
      if (count_in_stats)
        find_node_success++;
      if (queries > 4)
        Engine.verbose("FIND_NODE on %08x success in %d steps", id_to_find, steps);
      routingTableUpdate(id_to_find);
    } else {
      if (count_in_stats) {
        find_node_failed++;
        Engine.verbose("%08x not found in %d steps", id_to_find, steps);
      }
    }
    return destination_found;
  }

  /** Does a pseudo-random lookup for someone in the system */
  void randomLookup()
  {
    long id_to_look = dht_kademlia.RANDOM_LOOKUP_NODE; // Totally random.
    /* TODO: Use some pseudo-random generator. */
    Engine.debug("I'm doing a random lookup");
    findNode(id_to_look, true);
  }

  /** Handles the answer to an incoming "find_node" message */
  void handleFindNode(Message msg)
  {
    routingTableUpdate(msg.sender_id);
    Engine.verbose("Received a FIND_NODE from %s (%s), he's trying to find %08x", msg.answer_to.get_name(),
                   msg.issuer_host_name, msg.destination_id);
    // Building the answer to the request
    Message answer = new Message(id, msg.destination_id, findClosest(msg.destination_id),
                                 actor.get_engine().mailbox_by_name(String.valueOf(id)), actor.get_host().get_name());
    // Sending the answer
    msg.answer_to.put_init(answer, 1).detach();
  }

  void displaySuccessRate()
  {
    Engine.info("%d/%d FIND_NODE have succeeded", find_node_success, find_node_success + find_node_failed);
  }
}

/* ------------------------------------------------------------------------ */
/* Main code of an actor                                                    */
/* ------------------------------------------------------------------------ */

class NodeActor extends Actor {
  Node node;
  long known_id = -1;
  double deadline;
  boolean has_known_id;

  /** Exposes the protected Actor.sleep_for() to the plain Node object that drives this actor. */
  public void sleepFor(double duration) { this.sleep_for(duration); }

  /** @param args my node ID, the ID of the person I know in the system (or not), and the time before I leave */
  public NodeActor(String[] args)
  {
    if (args.length != 2 && args.length != 3)
      Engine.die("Wrong number of arguments");

    /* Node initialization */
    long node_id = Long.decode(args[0]);
    node         = new Node(node_id, this);

    has_known_id = args.length == 3;
    if (has_known_id) {
      known_id = Long.decode(args[1]);
      deadline = Double.parseDouble(args[2]);
    } else {
      deadline = Double.parseDouble(args[1]);
    }
  }

  public void run() throws SimgridException
  {
    boolean join_success = true;

    if (has_known_id) {
      Engine.info("Hi, I'm going to join the network with id %d", node.getId());
      join_success = node.join(known_id);
    } else {
      Engine.info("Hi, I'm going to create the network with id %d", node.getId());
      node.routingTableUpdate(node.getId());
    }

    if (join_success) {
      Engine.verbose("Ok, I'm joining the network with id %d", node.getId());
      // We start the main loop
      double next_lookup_time = Engine.get_clock() + dht_kademlia.RANDOM_LOOKUP_INTERVAL;

      Engine.verbose("Main loop start");

      Mailbox mailbox = this.get_engine().mailbox_by_name(String.valueOf(node.getId()));

      while (Engine.get_clock() < deadline) {
        Message msg = node.receive(mailbox);
        if (msg != null) {
          // There has been a message, we need to handle it !
          node.handleFindNode(msg);
        } else {
          /* We search for a pseudo random node */
          if (Engine.get_clock() >= next_lookup_time) {
            node.randomLookup();
            next_lookup_time += dht_kademlia.RANDOM_LOOKUP_INTERVAL;
          } else {
            // Didn't get a message: sleep for a while...
            this.sleep_for(1);
          }
        }
      }
    } else {
      Engine.info("I couldn't join the network :(");
    }
    Engine.debug("I'm leaving the network");
    node.displaySuccessRate();
  }
}
