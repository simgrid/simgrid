/* Copyright (c) 2009-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>
#include <vector>
#include <regex>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include <simgrid/msg.h>
#include <smpi/smpi.h>

using namespace std;

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Messages specific for this example");

void pop_some_processes(int nb_processes, msg_host_t host);

struct Job
{
    string smpi_app_name;
    string filename;
    int app_size;
    int starting_time;
    vector<int> allocation;
    vector<string> traces_filenames;
    int unique_job_number; // in [0, n[
};

// ugly global to avoid creating structures for giving args to processes
std::vector<msg_host_t> hosts;

// Comparator to sort jobs
bool operator<(const Job & j1, const Job & j2)
{
    if (j1.starting_time == j2.starting_time)
        return j1.smpi_app_name < j2.smpi_app_name;
    return j1.starting_time < j2.starting_time;
}

struct s_smpi_replay_process_args
{
    Job * job;
    msg_sem_t semaphore;
    int rank;
};

int smpi_replay_process(int argc, char *argv[])
{
    s_smpi_replay_process_args * args = (s_smpi_replay_process_args *)
                                    MSG_process_get_data(MSG_process_self());

    if (args->semaphore != nullptr)
        MSG_sem_acquire(args->semaphore);

    XBT_INFO("Replaying rank %d of job %d (smpi_app '%s')", args->rank,
             args->job->unique_job_number, args->job->smpi_app_name.c_str());
    // Added on hack commit 43c69dc808 on https://github.com/mpoquet/simgrid
    //smpi_replay_run_with_index(&argc, &argv, args->job->unique_job_number);
    // Otherwise, the following function can be called
    smpi_replay_run(&argc, &argv);
    XBT_INFO("Finished replaying rank %d of job %d (smpi_app '%s')", args->rank,
             args->job->unique_job_number, args->job->smpi_app_name.c_str());

    if (args->semaphore != nullptr)
        MSG_sem_release(args->semaphore);

    delete args;
    return 0;
}

// Sleeps for a given amount of time
int sleeper_process(int argc, char * argv[])
{
    (void) argc;
    (void) argv;

    int * param = (int*) MSG_process_get_data(MSG_process_self());

    XBT_INFO("Sleeping for %d seconds", *param);
    MSG_process_sleep((double) *param);

    delete param;

    return 0;
}

int job_executor_process(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    Job * job = (Job *) MSG_process_get_data(MSG_process_self());

    msg_sem_t job_semaphore = MSG_sem_init(1);
    XBT_INFO("Executing job %d (smpi_app '%s')",
             job->unique_job_number, job->smpi_app_name.c_str());

    for (int i = 0; i < job->app_size; ++i)
    {
        // Probable memory leaks
        char * str_instance_name, * str_rank, * str_pname, * str_tfname;
        asprintf(&str_instance_name, "%s", job->smpi_app_name.c_str());
        asprintf(&str_rank, "%d", i);
        asprintf(&str_pname, "%d_%d", job->unique_job_number, i);
        asprintf(&str_tfname, "%s", job->traces_filenames[i].c_str());

        char **argv = xbt_new(char*, 5);
        argv[0] = xbt_strdup("1");      // log only?
        argv[1] = str_instance_name;    // application instance
        argv[2] = str_rank;             // rank
        argv[3] = str_tfname;           // smpi trace file for this rank
        argv[4] = xbt_strdup("0");      // ?

        s_smpi_replay_process_args * args = new s_smpi_replay_process_args;
        args->job = job;
        args->semaphore = nullptr;
        args->rank = i;

        if (i == 0)
            args->semaphore = job_semaphore;

        MSG_process_create_with_arguments(str_pname, smpi_replay_process,
                                          (void*) args,
                                          hosts[job->allocation[i]],
                                          5, argv);
    }

    MSG_sem_acquire(job_semaphore);
    MSG_sem_destroy(job_semaphore);

    XBT_INFO("Finished job %d (smpi_app '%s')",
             job->unique_job_number, job->smpi_app_name.c_str());

    return 0;
}

// Executes a workload of SMPI processes
int workload_executor_process(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    vector<Job*> * workload = (vector<Job*> *) MSG_process_get_data(MSG_process_self());
    double curr_time;

    for (const Job * job : *workload)
    {
        // Let's wait until the job's waiting time if needed
        curr_time = MSG_get_clock();
        if (job->starting_time > curr_time)
        {
            double time_to_sleep = (double) job->starting_time - curr_time;
            XBT_INFO("Sleeping %g seconds (waiting for job %d, app '%s')",
                     time_to_sleep, job->starting_time,
                     job->smpi_app_name.c_str());
            MSG_process_sleep(time_to_sleep);
        }

        // Let's add some process noise
        /*XBT_INFO("Popping %d noise processes before running job %d (app '%s')",
                 job->app_size, job->unique_job_number,
                 job->smpi_app_name.c_str());
        pop_some_processes(job->app_size, hosts[job->allocation[0]]);*/

        // Let's finally run the job executor
        string job_process_name = "job_" + job->smpi_app_name;
        XBT_INFO("Launching the job executor of job %d (app '%s')",
                 job->unique_job_number, job->smpi_app_name.c_str());
        MSG_process_create(job_process_name.c_str(), job_executor_process,
                           (void*) job, hosts[job->allocation[0]]);
    }

    return 0;
}

// Launches some sleeper processes
void pop_some_processes(int nb_processes, msg_host_t host)
{
    for (int i = 0; i < nb_processes; ++i)
    {
        int * param = new int;
        *param = i + 1;
        MSG_process_create("meh", sleeper_process, (void*) param, host);
    }
}

// Reads jobs from a workload file and returns them
vector<Job *> all_jobs(const std::string & workload_file)
{
    ifstream f(workload_file);
    xbt_assert(f.is_open(), "Cannot open file '%s'.", workload_file.c_str());
    vector<Job *> jobs;

    regex r(R"(^\s*(\S+)\s+(\S+\.txt)\s+(\d+)\s+(\d+)\s+(\d+(?:,\d+)*).*$)");
    string line;
    while (getline(f, line))
    {
        smatch m;

        if (regex_match(line, m, r))
        {
            try
            {
                Job * job = new Job;
                job->smpi_app_name = m[1];
                job->filename = m[2];
                job->app_size = stoi(m[3]);
                job->starting_time = stoi(m[4]);
                string alloc = m[5];

                vector<string> subparts;
                boost::split(subparts, alloc, boost::is_any_of(","),
                             boost::token_compress_on);

                if ((int) subparts.size() != job->app_size)
                    throw std::runtime_error("size/alloc inconsistency");

                job->allocation.resize(subparts.size());
                for (unsigned int i = 0; i < subparts.size(); ++i)
                    job->allocation[i] = stoi(subparts[i]);

                // Let's read the filename
                ifstream traces_file(job->filename);
                if (!traces_file.is_open())
                    throw std::runtime_error("Cannot open file "+job->filename);

                string traces_line;
                while (getline(traces_file, traces_line))
                {
                    boost::trim_right(traces_line);
                    job->traces_filenames.push_back(traces_line);
                }

                if ((int) job->traces_filenames.size() < job->app_size)
                    throw std::runtime_error("size/tracefiles inconsistency");
                job->traces_filenames.resize(job->app_size);

                printf("Job read: app='%s', file='%s', size=%d, start=%d, "
                       "alloc='%s'\n", job->smpi_app_name.c_str(),
                       job->filename.c_str(), job->app_size, job->starting_time,
                       alloc.c_str());
                jobs.push_back(job);
            }
            catch(const std::exception & e)
            {
                printf("Bad line '%s' of file '%s': %s.\n",
                       line.c_str(), workload_file.c_str(),
                       e.what());
            }
        }
    }

    // Jobs are sorted by ascending date, then by lexicographical order of their
    // application names
    sort(jobs.begin(), jobs.end());

    for (unsigned int i = 0; i < jobs.size(); ++i)
        jobs[i]->unique_job_number = i;

    return jobs;
}

// Returns all MSG hosts as a std::vector
vector<msg_host_t> all_hosts()
{
    vector<msg_host_t> hosts;

    xbt_dynar_t hosts_dynar = MSG_hosts_as_dynar();
    msg_host_t host;
    unsigned int i;
    xbt_dynar_foreach(hosts_dynar, i, host)
    {
        hosts.push_back(host);
    }

    xbt_dynar_free(&hosts_dynar);

    return hosts;
}

int main(int argc, char *argv[])
{
    MSG_init(&argc, argv);

    xbt_assert(argc > 2, "Usage: %s platform_file workload_file\n"
               "\tExample: %s platform.xml workload_compute\n",
               argv[0], argv[0]);

    //  Simulation setting
    MSG_create_environment(argv[1]);
    hosts = all_hosts();

    xbt_assert(hosts.size() >= 4, "The given platform should contain at least "
                                  "4 hosts (found %lu).", hosts.size());

    // Let's retrieve all SMPI jobs
    vector<Job*> jobs = all_jobs(argv[2]);

    // Let's register them
    for (const Job * job : jobs)
        SMPI_app_instance_register(job->smpi_app_name.c_str(),
                                   smpi_replay_process, job->app_size);

    SMPI_init();

    // Initial noise can be added
    //pop_some_processes(3000, hosts[0]);

    // Let's execute the workload
    MSG_process_create("workload_executor", workload_executor_process,
                       (void*) &jobs, hosts[0]);

    msg_error_t res = MSG_main();

    XBT_INFO("Simulation finished! Final time: %g", MSG_get_clock());

    SMPI_finalize();

    for (const Job * job : jobs)
        delete job;

    return res != MSG_OK;
}
