/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;


class Sender extends ActorMain {
  static String msg1 = "msg1= Salut mon pote";
  public void run() {
    Mailbox mb = Mailbox.by_name("hello");
    mb.put(msg1, 100000);
    Engine.info("Sender: msg1 sent");
    String msg2 = "msg2= Et la?";
    mb.put(msg2, 100000);
    Engine.info("Sender: msg2 sent; terminating");
  }
}
class Receiver extends ActorMain {
  public void run() {
    Mailbox mb = Mailbox.by_name("hello");
    Object res = mb.get();
    Engine.info("Receiver: Got1 '"+((String)res)+"'");
    Object res2 = mb.get();
    Engine.info("Receiver: Got2 '"+((String)res2)+"'"); 
  }
}

public class simgrid_runme {
  
  public static void main(String[] args) {

    Engine e = Engine.get_instance(args);
    e.load_platform("small_platform.xml");

    Actor.create("sender", e.host_by_name("Ginette"), new Sender());
    Actor.create("receiver", e.host_by_name("Fafard"), new Receiver());

    e.run();
    Engine.info("The simulation is terminating");
  }

}
