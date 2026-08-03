/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This source code simply loads the platform. This is only useful to play
 * with the tracing module. See the tesh file to see how to generate the
 * traces.
 */

import org.simgrid.s4u.*;

public class trace_platform {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    args     = e.get_args(); // get ride of any configuration flag on the command line

    e.load_platform(args[0]);
    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
