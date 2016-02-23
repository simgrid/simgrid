/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package kademlia;
import java.util.Collections;
import java.util.Vector;

import org.simgrid.msg.Msg;

public class RoutingTable {
  /* Bucket list */
  private Vector<Bucket> buckets;
  /* Id of the routing table owner */
  private int id;

  public RoutingTable(int id) {
    this.id = id;
    buckets = new Vector<Bucket>();
    for (int i = 0; i < Common.IDENTIFIER_SIZE + 1; i++) {
      buckets.add(new Bucket(i));
    }
  }

  /**
   * @brief Returns an identifier which is in a specific bucket of a routing table
   * @param id id of the routing table owner
   * @param prefix id of the bucket where we want that identifier to be
   */
  public int getIdInPrefix(int id, int prefix) {
    if (prefix == 0) {
      return 0;
    }
    int identifier = 1;
    identifier = identifier << (prefix - 1);
    identifier = identifier ^ id;
    return identifier;
  }

  /* Returns the corresponding node prefix for a given id */
  public int getNodePrefix(int id) {
    for (int j = 0; j < 32; j++) {
      if ((id >> (32 - 1 - j) & 0x1) != 0) {
        return 32 - j;
      }
    }
    return 0;
  }

  /* Finds the corresponding bucket in a routing table for a given identifier */
  public Bucket findBucket(int id) {
    int xorNumber = id ^ this.id;
    int prefix = this.getNodePrefix(xorNumber);
    return buckets.get(prefix);
  }

  /* Updates the routing table with a new value. */
  public void update(int id) {
    Bucket bucket = this.findBucket(id);
    if (bucket.contains(id)) {
      Msg.debug("Updating " + Integer.toString(id) + " in my routing table");
      //If the element is already in the bucket, we update it.
      bucket.pushToFront(id);
    } else {
      Msg.debug("Adding " + id + " to my routing table");
      bucket.add(id);
      if (bucket.size() > Common.BUCKET_SIZE)  {
        //TODO: Ping the least seen guy and remove him if he is offline.
      }
    }
  }

  /* Returns the closest notes we know to a given id */
  public Answer findClosest(int destinationId) {
    Answer answer = new Answer(destinationId);
    Bucket bucket = this.findBucket(destinationId);
    bucket.addToAnswer(answer,destinationId);

    for (int i = 1; answer.size() < Common.BUCKET_SIZE && ((bucket.getId() - i) >= 0 ||
                                    (bucket.getId() + i) <= Common.IDENTIFIER_SIZE); i++) {
      //Check the previous buckets
      if (bucket.getId() - i >= 0) {
        Bucket bucketP = this.buckets.get(bucket.getId() - i);
        bucketP.addToAnswer(answer,destinationId);
      }
      //Check the next buckets
      if (bucket.getId() + i <= Common.IDENTIFIER_SIZE) {
        Bucket bucketN = this.buckets.get(bucket.getId() + i);
        bucketN.addToAnswer(answer, destinationId);
      }
    }
    //We sort the list
    Collections.sort(answer.getNodes());
    //We trim the list
    while (answer.size() > Common.BUCKET_SIZE) {
      answer.remove(answer.size() - 1); //TODO: Not the best thing.
    }
    return answer;
  }

  @Override
  public String toString() {
    String string = "RoutingTable [ id=" + id + " " ;
    for (int i = 0; i < buckets.size(); i++) {
      if (buckets.get(i).size() > 0) {
        string += buckets.get(i) + " ";
      }
    }
    return string;
  }
}
