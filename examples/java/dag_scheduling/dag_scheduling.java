/* Copyright (c) 2009-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* simple test to schedule a DAX file with the Min-Min algorithm.           */

import java.util.HashMap;
import java.util.Map;
import java.util.Vector;
import org.simgrid.s4u.*;

/* Java cannot return pairs of value, so we have to build such a pair explicitely */
class SchedulingDecision {
  Host host;
  double min_finish_time;
}

public class dag_scheduling {

  static Vector<Exec> get_ready_tasks(Activity[] dax)
  {
    Vector<Exec> ready_tasks           = new Vector<>();
    Map<Exec, Integer> candidate_execs = new HashMap<>();

    for (Activity a : dax) {
      // Only look at activity that have their dependencies solved but are not assigned
      if (a.dependencies_solved() && !a.is_assigned()) {
        // if it is an exec, it's ready
        if (a instanceof Exec)
          ready_tasks.add((Exec)a);
        // if it a comm, we consider its successor as a candidate. If a candidate solves all its dependencies,
        // i.e., get all its input data, it's ready
        if (a instanceof Comm) {
          Exec next_exec = (Exec)((Comm)a).get_successors()[0];
          candidate_execs.put(next_exec, candidate_execs.getOrDefault(next_exec, 0) + 1);
          if (next_exec.get_dependencies().length == candidate_execs.get(next_exec)) {
            ready_tasks.add(next_exec);
            Engine.critical("next_exec is schedulable");
          }
        }
      }
    }
    return ready_tasks;
  }

  static SchedulingDecision get_best_host(Exec exec)
  {
    SchedulingDecision res = new SchedulingDecision();
    res.host               = null;
    res.min_finish_time    = Double.MAX_VALUE;

    for (Host host : Engine.get_instance().get_all_hosts()) {
      double data_available      = 0.;
      double last_data_available = -1.0;
      /* compute last_data_available */
      for (Activity parent : exec.get_dependencies()) {
        /* normal case */
        if (parent instanceof Comm) {
          Comm comm   = (Comm)parent;
          Host source = comm.get_source();
          Engine.debug("transfer from %s to %s", source.get_name(), host.get_name());
          /* Estimate the redistribution time from this parent */
          double redist_time;
          if (comm.get_remaining() <= 1e-6) {
            redist_time = 0;
          } else {
            double bandwidth = Double.MAX_VALUE;
            Link[] links     = source.route_links_to(host);
            double latency   = source.route_latency_to(host);
            for (Link link : links)
              bandwidth = Math.min(bandwidth, link.get_bandwidth());

            redist_time = latency + comm.get_remaining() / bandwidth;
          }
          // We use the user data field to store the finish time of the predecessor of the comm, i.e., its potential
          // start time
          data_available = (Double)comm.get_data() + redist_time;
        }

        /* no transfer, control dependency */
        if (parent instanceof Exec)
          data_available = ((Exec)parent).get_finish_time();

        if (last_data_available < data_available)
          last_data_available = data_available;
      }

      double finish_time =
          Math.max((Double)host.get_data(), last_data_available) + exec.get_remaining() / host.get_speed();

      Engine.debug("%s finishes on %s at %f", exec.get_name(), host.get_name(), finish_time);

      if (finish_time < res.min_finish_time) {
        res.min_finish_time = finish_time;
        res.host            = host;
      }
    }

    return res;
  }

  static void schedule_on(Exec exec, Host host, double busy_until)
  {
    exec.set_host(host);
    // We use the user data field to store up to when the host is busy
    host.set_data(Double.valueOf(busy_until));
    // we can also set the destination of all the input comms of this exec
    for (Activity succ : exec.get_successors()) {
      if (succ instanceof Comm) {
        Comm comm = (Comm)succ;
        comm.set_destination(host);
        comm.set_data(null);
      }
    }
    // we can also set the source of all the output comms of this exec
    for (Activity succ : exec.get_successors()) {
      if (succ instanceof Comm) {
        ((Comm)succ).set_source(host);
      }
    }
  }
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.track_vetoed_activities();

    Exec.on_completion_cb(new CallbackExec() {
      public void run(Exec exec)
      {
        Engine.info("Task %s completes", exec.get_name());
        // when an Exec completes, we need to set the potential start time of all its ouput comms
        for (Activity succ : exec.get_successors()) {
          if (succ instanceof Comm) {
            Double finish_time = Double.valueOf(exec.get_finish_time());
            // We use the user data field to store the finish time of the predecessor of the comm, i.e., its potential
            // start time
            succ.set_data(finish_time);
          }
        }
      }
    });

    e.load_platform(args[0]);

    /* Mark all hosts as sequential, as it ought to be in such a scheduling example.
     *
     * It means that the hosts can only compute one thing at a given time. If an execution already takes place on a
     * given host, any subsequently started execution will be queued until after the first execution terminates */
    for (Host host : e.get_all_hosts()) {
      host.set_concurrency_limit(1);
      host.set_data(Double.valueOf(0.0));
    }
    /* load the DAX file */
    Activity[] dax = e.create_DAG_from_DAX(args[1]);

    /* Schedule the root first */
    Exec root                   = (Exec)dax[0];
    SchedulingDecision decision = get_best_host(root);
    schedule_on(root, decision.host, 0);

    e.run();

    while (e.get_vetoed_activities().length > 0) {
      Engine.debug("Start new scheduling round");
      /* Get the set of ready tasks */
      Vector<Exec> ready_tasks = get_ready_tasks(dax);
      e.clear_vetoed_activities();

      if (ready_tasks.size() == 0) {
        Engine.debug("There is no ready exec, let's advance the simulation.");
        e.run();
        continue;
      } else {
        Engine.debug("There are %d ready exec, let's schedule them.", ready_tasks.size());
      }
      /* For each ready exec:
       * get the host that minimizes the completion time.
       * select the exec that has the minimum completion time on its best host.
       */
      double min_finish_time = Double.MAX_VALUE;
      Exec selected_task     = null;
      Host selected_host     = null;

      for (Exec exec : ready_tasks) {
        decision = get_best_host(exec);
        if (decision.min_finish_time < min_finish_time) {
          min_finish_time = decision.min_finish_time;
          selected_task   = exec;
          selected_host   = decision.host;
        }
      }

      Engine.info("Schedule %s on %s", selected_task.get_name(), selected_host.get_name());
      schedule_on(selected_task, selected_host, min_finish_time);

      ready_tasks.clear();
      e.run();
    }

    /* Cleanup memory */
    for (Host h : e.get_all_hosts())
      h.set_data(null);

    Engine.info("Simulation ends.");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}