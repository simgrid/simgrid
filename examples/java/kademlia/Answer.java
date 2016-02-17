/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package kademlia;
import java.util.ArrayList;
import java.util.Collections;

/* Answer to a "FIND_NODE" query. Contains the nodes closest to an id given */
public class Answer {
  private int destinationId;
  /* Closest nodes in the answer. */
  private ArrayList<Contact> nodes;

  public Answer(int destinationId) {
    this.destinationId = destinationId;
    nodes = new ArrayList<Contact>();
  }

  int getDestinationId() {
    return destinationId;
  }

  ArrayList<Contact> getNodes() {
    return nodes;
  }

  int size() {
    return nodes.size();
  }

  public void remove(int index) {
    nodes.remove(index);
  }

  public void add(Contact contact) {
    nodes.add(contact);
  }

  /* Merge the contents of this answer with another answer */
  public int merge(Answer answer) {
    int nbAdded = 0;

    for (Contact c: answer.getNodes()) {
      if (!nodes.contains(c)) {
        nbAdded++;
        nodes.add(c);
      }
    }
    Collections.sort(nodes);
    //Trim the list
    while (answer.size() > Common.BUCKET_SIZE) {
      answer.remove(answer.size() - 1);
    }
    return nbAdded;
  }

  /* Returns if the destination has been found */
  public boolean destinationFound() {
    if (nodes.size() < 1) {
      return false;
    }
    Contact tail = nodes.get(0);
    return tail.getDistance() == 0;
  }

  @Override
  public String toString() {
    return "Answer [destinationId=" + destinationId + ", nodes=" + nodes + "]";
  }
}
