/* Copyright (c) 2010, 2012-2015, 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.hpp"
#include "xbt/log.h"
#include "xbt/replay.hpp"

#include <boost/algorithm/string.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(replay,xbt,"Replay trace reader");

namespace simgrid {
namespace xbt {

std::ifstream* action_fs = nullptr;
std::unordered_map<std::string, action_fun> action_funs;

static void read_and_trim_line(std::ifstream* fs, std::string* line)
{
  do {
    std::getline(*fs, *line);
    boost::trim(*line);
  } while (not fs->eof() && (line->length() == 0 || line->front() == '#'));
  XBT_DEBUG("got from trace: %s", line->c_str());
}

class ReplayReader {
  std::ifstream* fs;
  std::string line;

public:
  explicit ReplayReader(const char* filename)
  {
    XBT_VERB("Prepare to replay file '%s'", filename);
    fs = new std::ifstream(filename, std::ifstream::in);
    xbt_assert(fs->is_open(), "Cannot read replay file '%s'", filename);
  }
  ~ReplayReader()
  {
    delete fs;
  }
  bool get(ReplayAction* action);
};

bool ReplayReader::get(ReplayAction* action)
{
  read_and_trim_line(fs, &line);

  boost::split(*action, line, boost::is_any_of(" \t"), boost::token_compress_on);
  return not fs->eof();
}

static ReplayAction* get_action(char* name)
{
  ReplayAction* action;

  std::queue<ReplayAction*>* myqueue = nullptr;
  if (action_queues.find(std::string(name)) != action_queues.end())
    myqueue = action_queues.at(std::string(name));
  if (myqueue == nullptr || myqueue->empty()) { // Nothing stored for me. Read the file further
    // Read lines until I reach something for me (which breaks in loop body) or end of file reached
    while (true) {
      std::string action_line;
      read_and_trim_line(action_fs, &action_line);
      if (action_fs->eof())
        break;
      /* we cannot split in place here because we parse&store several lines for the colleagues... */
      action = new ReplayAction();
      boost::split(*action, action_line, boost::is_any_of(" \t"), boost::token_compress_on);

      // if it's for me, I'm done
      std::string evtname = action->front();
      if (evtname.compare(name) == 0) {
        return action;
      } else {
        // Else, I have to store it for the relevant colleague
        std::queue<ReplayAction*>* otherqueue = nullptr;
        if (action_queues.find(evtname) != action_queues.end())
          otherqueue = action_queues.at(evtname);
        else { // Damn. Create the queue of that guy
          otherqueue = new std::queue<ReplayAction*>();
          action_queues.insert({evtname, otherqueue});
        }
        otherqueue->push(action);
      }
    }
    // end of file reached while searching in vain for more work
  } else {
    // Get something from my queue and return it
    action = myqueue->front();
    myqueue->pop();
    return action;
  }
  return nullptr;
}

static void handle_action(ReplayAction* action)
{
  XBT_DEBUG("%s replays a %s action", action->at(0).c_str(), action->at(1).c_str());
  char** c_action     = new char*[action->size() + 1];
  action_fun function = action_funs.at(action->at(1));
  int i               = 0;
  for (auto arg : *action) {
    c_action[i] = xbt_strdup(arg.c_str());
    i++;
  }
  c_action[i] = nullptr;
  try {
    function(c_action);
  } catch (xbt_ex& e) {
    for (unsigned int j = 0; j < action->size(); j++)
      xbt_free(c_action[j]);
    delete[] c_action;
    action->clear();
    xbt_die("Replay error:\n %s", e.what());
  }
  for (unsigned int j = 0; j < action->size(); j++)
    xbt_free(c_action[j]);
  delete[] c_action;
}

/**
 * \ingroup XBT_replay
 * \brief function used internally to actually run the replay
 */
int replay_runner(int argc, char* argv[])
{
  if (simgrid::xbt::action_fs) { // A unique trace file
    while (true) {
      simgrid::xbt::ReplayAction* evt = simgrid::xbt::get_action(argv[0]);
      if (evt == nullptr)
        break;
      simgrid::xbt::handle_action(evt);
      delete evt;
    }
    if (action_queues.find(std::string(argv[0])) != action_queues.end()) {
      std::queue<ReplayAction*>* myqueue = action_queues.at(std::string(argv[0]));
      delete myqueue;
      action_queues.erase(std::string(argv[0]));
    }
  } else { // Should have got my trace file in argument
    simgrid::xbt::ReplayAction* evt = new simgrid::xbt::ReplayAction();
    xbt_assert(argc >= 2, "No '%s' agent function provided, no simulation-wide trace file provided, "
                          "and no process-wide trace file provided in deployment file. Aborting.",
               argv[0]);
    simgrid::xbt::ReplayReader* reader = new simgrid::xbt::ReplayReader(argv[1]);
    while (reader->get(evt)) {
      if (evt->front().compare(argv[0]) == 0) {
        simgrid::xbt::handle_action(evt);
      } else {
        XBT_WARN("Ignore trace element not for me");
      }
      evt->clear();
    }
    delete evt;
    delete reader;
  }
  return 0;
}
}
}

/**
 * \ingroup XBT_replay
 * \brief Registers a function to handle a kind of action
 *
 * Registers a function to handle a kind of action
 * This table is then used by \ref xbt_replay_action_runner
 *
 * The argument of the function is the line describing the action, fields separated by spaces.
 *
 * \param action_name the reference name of the action.
 * \param function prototype given by the type: void...(const char** action)
 */
void xbt_replay_action_register(const char* action_name, action_fun function)
{
    /* Using operator[] because std::unordered_map::insert does not allow to
     * replace the content associted to an already existing key. */
    simgrid::xbt::action_funs[std::string(action_name)] = function;
}

/**
 * \ingroup XBT_replay
 * \brief Get the function that was previously registered to handle a kind of action
 *
 * This can be useful if you want to override and extend an existing action.
 */
action_fun xbt_replay_action_get(const char* action_name)
{
  return simgrid::xbt::action_funs.at(std::string(action_name));
}
