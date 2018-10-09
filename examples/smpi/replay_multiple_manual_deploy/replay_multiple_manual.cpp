/* Copyright (c) 2009-2018. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to replay SMPI time-independent traces in a dynamic
   fashion. It is inspired from Batsim (https://github.com/oar-team/batsim).

   The program workflow can be summarized as:
   1. Read an input workload (set of jobs).
      Each job is a time-independent trace and a starting time.
   2. Create initial noise, by spawning useless actors.
      This is done to avoid SMPI actors to start at actor_id=0.
   3. For each job:
        1. Sleep until job's starting time is reached (if needed)
        2. Launch the replay of the corresponding time-indepent trace.
        3. Create inter-process noise, by spawning useless actors.
   4. Wait for completion (via s4u::Engine's run method)
*/

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <boost/algorithm/string.hpp>

#include <simgrid/msg.h>
#include <simgrid/s4u.hpp>
#include <smpi/smpi.h>
#include <xbt/file.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(replay_multiple_manual, "Messages specific for this example");

struct Job {
  std::string smpi_app_name;   //!< The unique name of the SMPI application
  std::string filename;        //!<  The filename of the main trace file (which contains other filenames for each rank)
  int app_size;                //!< The number of processes (actors) of the job
  int starting_time;           //!< When the job should start
  std::vector<int> allocation; //!< Where the job should be executed. Values are hosts indexes.
  std::vector<std::string> traces_filenames; //!< The filenames of the different action files. Read from filename.
  int unique_job_number;                     //!< The job unique number in [0, n[.
};

// ugly globals to avoid creating structures for giving args to processes
std::vector<simgrid::s4u::Host*> hosts;
int noise_between_jobs;

static bool job_comparator(const Job* j1, const Job* j2)
{
  if (j1->starting_time == j2->starting_time)
    return j1->smpi_app_name < j2->smpi_app_name;
  return j1->starting_time < j2->starting_time;
}

struct s_smpi_replay_process_args {
  Job* job;
  simgrid::s4u::BarrierPtr barrier;
  int rank;
};

static int smpi_replay_process(int argc, char* argv[])
{
  s_smpi_replay_process_args* args = static_cast<s_smpi_replay_process_args*>(MSG_process_get_data(MSG_process_self()));

  XBT_INFO("Replaying rank %d of job %d (smpi_app '%s')", args->rank, args->job->unique_job_number,
           args->job->smpi_app_name.c_str());

  smpi_replay_run(&argc, &argv);
  XBT_INFO("Finished replaying rank %d of job %d (smpi_app '%s')", args->rank, args->job->unique_job_number,
           args->job->smpi_app_name.c_str());

  args->barrier->wait();

  delete args;
  return 0;
}

// Sleeps for a given amount of time
static int sleeper_process(int* param)
{
  XBT_DEBUG("Sleeping for %d seconds", *param);
  simgrid::s4u::this_actor::sleep_for(*param);

  delete param;

  return 0;
}

// Launches some sleeper processes
static void pop_some_processes(int nb_processes, simgrid::s4u::Host* host)
{
  for (int i = 0; i < nb_processes; ++i) {
    int* param = new int;
    *param     = i + 1;
    simgrid::s4u::Actor::create("meh", host, sleeper_process, param);
  }
}

static int job_executor_process(Job* job)
{
  XBT_INFO("Executing job %d (smpi_app '%s')", job->unique_job_number, job->smpi_app_name.c_str());

  simgrid::s4u::BarrierPtr barrier = simgrid::s4u::Barrier::create(job->app_size + 1);

  for (int i = 0; i < job->app_size; ++i) {
    char** argv = xbt_new(char*, 5);
    argv[0]     = xbt_strdup("1");                              // log only?
    argv[1]     = xbt_strdup(job->smpi_app_name.c_str());       // application instance
    argv[2]     = bprintf("%d", i);                             // rank
    argv[3]     = xbt_strdup(job->traces_filenames[i].c_str()); // smpi trace file for this rank
    argv[4]     = xbt_strdup("0");   // ?

    s_smpi_replay_process_args* args = new s_smpi_replay_process_args;
    args->job                        = job;
    args->barrier                    = barrier;
    args->rank                       = i;

    char* str_pname = bprintf("%d_%d", job->unique_job_number, i);
    MSG_process_create_with_arguments(str_pname, smpi_replay_process, (void*)args, hosts[job->allocation[i]], 5, argv);
    xbt_free(str_pname);
  }

  barrier->wait();

  XBT_INFO("Finished job %d (smpi_app '%s')", job->unique_job_number, job->smpi_app_name.c_str());

  return 0;
}

// Executes a workload of SMPI processes
static int workload_executor_process(std::vector<Job*>* workload)
{
  for (Job* job : *workload) {
    // Let's wait until the job's waiting time if needed
    double curr_time = simgrid::s4u::Engine::get_clock();
    if (job->starting_time > curr_time) {
      double time_to_sleep = (double)job->starting_time - curr_time;
      XBT_INFO("Sleeping %g seconds (waiting for job %d, app '%s')", time_to_sleep, job->starting_time,
               job->smpi_app_name.c_str());
      simgrid::s4u::this_actor::sleep_for(time_to_sleep);
    }

    if (noise_between_jobs > 0) {
      // Let's add some process noise
      XBT_DEBUG("Popping %d noise processes before running job %d (app '%s')", noise_between_jobs,
                job->unique_job_number, job->smpi_app_name.c_str());
      pop_some_processes(noise_between_jobs, hosts[job->allocation[0]]);
    }

    // Let's finally run the job executor
    std::string job_process_name = "job_" + job->smpi_app_name;
    XBT_INFO("Launching the job executor of job %d (app '%s')", job->unique_job_number, job->smpi_app_name.c_str());
    simgrid::s4u::Actor::create(job_process_name.c_str(), hosts[job->allocation[0]], job_executor_process, job);
  }

  return 0;
}

// Reads jobs from a workload file and returns them
static std::vector<Job*> all_jobs(const std::string& workload_file)
{
  std::ifstream f(workload_file);
  xbt_assert(f.is_open(), "Cannot open file '%s'.", workload_file.c_str());
  std::vector<Job*> jobs;

  simgrid::xbt::Path path(workload_file);
  std::string dir = path.get_dir_name();

  std::string line;
  while (std::getline(f, line)) {
    std::string app_name;
    std::string filename_unprefixed;
    int app_size;
    int starting_time;
    std::string alloc;

    std::istringstream is(line);
    if (is >> app_name >> filename_unprefixed >> app_size >> starting_time >> alloc) {
      try {
        Job job;
        job.smpi_app_name = app_name;
        job.filename      = dir + "/" + filename_unprefixed;
        job.app_size      = app_size;
        job.starting_time = starting_time;

        std::vector<std::string> subparts;
        boost::split(subparts, alloc, boost::is_any_of(","), boost::token_compress_on);

        if ((int)subparts.size() != job.app_size)
          throw std::invalid_argument("size/alloc inconsistency");

        job.allocation.resize(subparts.size());
        for (unsigned int i = 0; i < subparts.size(); ++i)
          job.allocation[i] = stoi(subparts[i]);

        // Let's read the filename
        std::ifstream traces_file(job.filename);
        if (!traces_file.is_open())
          throw std::invalid_argument("Cannot open file " + job.filename);

        std::string traces_line;
        while (std::getline(traces_file, traces_line)) {
          boost::trim_right(traces_line);
          job.traces_filenames.push_back(dir + "/" + traces_line);
        }

        if (static_cast<int>(job.traces_filenames.size()) < job.app_size)
          throw std::invalid_argument("size/tracefiles inconsistency");
        job.traces_filenames.resize(job.app_size);

        XBT_INFO("Job read: app='%s', file='%s', size=%d, start=%d, "
                 "alloc='%s'",
                 job.smpi_app_name.c_str(), filename_unprefixed.c_str(), job.app_size, job.starting_time,
                 alloc.c_str());
        jobs.push_back(new Job(std::move(job)));
      } catch (const std::invalid_argument& e) {
        xbt_die("Bad line '%s' of file '%s': %s.\n", line.c_str(), workload_file.c_str(), e.what());
      }
    }
  }

  // Jobs are sorted by ascending date, then by lexicographical order of their
  // application names
  sort(jobs.begin(), jobs.end(), job_comparator);

  for (unsigned int i = 0; i < jobs.size(); ++i)
    jobs[i]->unique_job_number = i;

  return jobs;
}

int main(int argc, char* argv[])
{
  xbt_assert(argc > 4,
             "Usage: %s platform_file workload_file initial_noise noise_between_jobs\n"
             "\tExample: %s platform.xml workload_compute\n",
             argv[0], argv[0]);

  //  Simulation setting
  MSG_init(&argc, argv);
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  hosts = e.get_all_hosts();
  xbt_assert(hosts.size() >= 4, "The given platform should contain at least 4 hosts (found %zu).", hosts.size());

  // Let's retrieve all SMPI jobs
  std::vector<Job*> jobs = all_jobs(argv[2]);

  // Let's register them
  for (const Job* job : jobs)
    SMPI_app_instance_register(job->smpi_app_name.c_str(), smpi_replay_process, job->app_size);

  SMPI_init();

  // Read noise arguments
  int initial_noise = std::stoi(argv[3]);
  xbt_assert(initial_noise >= 0, "Invalid initial_noise argument");

  noise_between_jobs = std::stoi(argv[4]);
  xbt_assert(noise_between_jobs >= 0, "Invalid noise_between_jobs argument");

  if (initial_noise > 0) {
    XBT_DEBUG("Popping %d noise processes", initial_noise);
    pop_some_processes(initial_noise, hosts[0]);
  }

  // Let's execute the workload
  simgrid::s4u::Actor::create("workload_executor", hosts[0], workload_executor_process, &jobs);

  e.run();
  XBT_INFO("Simulation finished! Final time: %g", e.get_clock());

  SMPI_finalize();

  for (const Job* job : jobs)
    delete job;

  return 0;
}
