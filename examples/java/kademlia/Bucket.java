/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package kademlia;
import java.util.ArrayList;

public class Bucket {
  private ArrayList<Integer> nodes;
  private int id;

  public Bucket(int id) {
    this.nodes = new ArrayList<Integer>();
    this.id = id;
  }

  public int getId() {
    return this.id;
  }

  public int size() {
    return nodes.size();
  }

  public boolean contains(int id) {
    return nodes.contains(id);
  }

  /* Add a node to the front of the bucket */
  public void add(int id) {
    nodes.add(0,id);
  }

  /* Push a node to the front of a bucket */
  public void pushToFront(int id) {
    int i = nodes.indexOf(id);
    nodes.remove(i);
    nodes.add(0, id);
  }

  public int getNode(int id) {
    return nodes.get(id);
  }

  /* Add the content of the bucket into a answer object. */
  public void addToAnswer(Answer answer, int destination) {
    for (int id : this.nodes) {
      answer.getNodes().add(new Contact(id,id ^ destination));
    }
  }

  @Override
  public String toString() {
    return "Bucket [id= " + id + " nodes=" + nodes + "]";
  }
}
