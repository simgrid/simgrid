/* Copyright (c) 2010, 2012-2015, 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#include "xbt/ex.hpp"
#include "xbt/log.h"
#include "xbt/replay.hpp"
#include "xbt/str.h"
#include "xbt/sysdep.h"

#include <boost/algorithm/string.hpp>
#include <ctype.h>
#include <errno.h>
#include <fstream>
#include <queue>
#include <unordered_map>
#include <wchar.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(replay,xbt,"Replay trace reader");

namespace simgrid {
namespace xbt {

bool is_replay_active = false;
typedef std::vector<std::string> ReplayAction;

bool replay_is_active()
{
  return is_replay_active;
}

class ReplayReader {
  std::ifstream* fs;
  std::string line;

public:
  char* filename_;
  int linenum = 0;

  explicit ReplayReader(const char* filename)
  {
    filename_ = xbt_strdup(filename);
    fs        = new std::ifstream(filename, std::ifstream::in);
  }
  ~ReplayReader()
  {
    free(filename_);
    delete fs;
  }
  bool get(ReplayAction* action);
};

bool ReplayReader::get(ReplayAction* action)
{
  std::getline(*fs, line);
  boost::trim(line);
  XBT_DEBUG("got from trace: %s", line.c_str());
  linenum++;

  if (line.length() > 0 && line.find("#") == std::string::npos) {
    boost::split(*action, line, boost::is_any_of(" \t"), boost::token_compress_on);
    return !fs->eof();
  } else {
    if (fs->eof())
      return false;
    else
      return this->get(action);
  }
}
std::ifstream* action_fs = nullptr;
}
}

std::unordered_map<std::string, action_fun> xbt_action_funs;

xbt_dict_t xbt_action_queues = nullptr;

bool is_replay_active = false;

static simgrid::xbt::ReplayAction* action_get_action(char* name);

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
void xbt_replay_action_register(const char *action_name, action_fun function)
{
  if (!is_replay_active) // If the user registers a function before the start
    _xbt_replay_action_init();
  xbt_action_funs.insert({std::string(action_name), function});
}

void _xbt_replay_action_init()
{
  if (!is_replay_active) {
    xbt_action_queues = xbt_dict_new_homogeneous(nullptr);
    is_replay_active  = true;
  }
}

void _xbt_replay_action_exit()
{
  xbt_dict_free(&xbt_action_queues);
  xbt_action_queues = nullptr;
}

/**
 * \ingroup XBT_replay
 * \brief function used internally to actually run the replay

 * \param argc argc .
 * \param argv argv
 */
int xbt_replay_action_runner(int argc, char *argv[])
{
  if (simgrid::xbt::action_fs) { // A unique trace file
    while (true) {
      simgrid::xbt::ReplayAction* evt = action_get_action(argv[0]);
      if (evt == nullptr)
        break;

      char** args = new char*[evt->size() + 1];
      int i       = 0;
      for (auto arg : *evt) {
        args[i] = xbt_strdup(arg.c_str());
        i++;
      }
      args[i]             = nullptr;
      action_fun function = xbt_action_funs.at(evt->at(1));

      try {
        function(args);
      }
      catch(xbt_ex& e) {
        xbt_die("Replay error :\n %s", e.what());
      }
      for (unsigned int j = 0; j < evt->size(); j++)
        xbt_free(args[j]);
      delete[] args;
      delete evt;
    }
  } else {                      // Should have got my trace file in argument
    simgrid::xbt::ReplayAction* evt = new simgrid::xbt::ReplayAction();
    xbt_assert(argc >= 2,
                "No '%s' agent function provided, no simulation-wide trace file provided, "
                "and no process-wide trace file provided in deployment file. Aborting.", argv[0]
        );
    simgrid::xbt::ReplayReader* reader = new simgrid::xbt::ReplayReader(argv[1]);
    while (reader->get(evt)) {
      if (evt->at(0).compare(argv[0]) == 0) {
        char** args = new char*[evt->size() + 1];
        int i       = 0;
        for (auto arg : *evt) {
          args[i] = xbt_strdup(arg.c_str());
          i++;
        }
        args[i]             = nullptr;
        action_fun function = xbt_action_funs.at(evt->at(1));
        try {
          function(args);
        } catch(xbt_ex& e) {
          for (unsigned int j = 0; j < evt->size(); j++)
            xbt_free(args[j]);
          delete[] args;
          evt->clear();
          xbt_die("Replay error on line %d of file %s :\n %s", reader->linenum, reader->filename_, e.what());
        }
        for (unsigned int j = 0; j < evt->size(); j++)
          xbt_free(args[j]);
        delete[] args;
      } else {
        XBT_WARN("%s:%d: Ignore trace element not for me", reader->filename_, reader->linenum);
      }
      evt->clear();
    }
    delete evt;
    delete reader;
  }
  return 0;
}

static simgrid::xbt::ReplayAction* action_get_action(char* name)
{
  simgrid::xbt::ReplayAction* action;

  std::queue<simgrid::xbt::ReplayAction*>* myqueue =
      (std::queue<simgrid::xbt::ReplayAction*>*)xbt_dict_get_or_null(xbt_action_queues, name);
  if (myqueue == nullptr || myqueue->empty()) { // nothing stored for me. Read the file further
    if (simgrid::xbt::action_fs == nullptr) {   // File closed now. There's nothing more to read. I'm out of here
      goto todo_done;
    }
    // Read lines until I reach something for me (which breaks in loop body) or end of file reached
    while (!simgrid::xbt::action_fs->eof()) {
      std::string action_line;
      std::getline(*simgrid::xbt::action_fs, action_line);
      // cleanup and split the string I just read
      boost::trim(action_line);
      XBT_DEBUG("got from trace: %s", action_line.c_str());
      if (action_line.length() > 0 && action_line.find("#") == std::string::npos) {
        /* we cannot split in place here because we parse&store several lines for the colleagues... */
        action = new simgrid::xbt::ReplayAction();
        boost::split(*action, action_line, boost::is_any_of(" \t"), boost::token_compress_on);

        // if it's for me, I'm done
        std::string evtname = action->front();
        if (evtname.compare(name) == 0) {
          return action;
        } else {
          // Else, I have to store it for the relevant colleague
          std::queue<simgrid::xbt::ReplayAction*>* otherqueue =
              (std::queue<simgrid::xbt::ReplayAction*>*)xbt_dict_get_or_null(xbt_action_queues, evtname.c_str());
          if (otherqueue == nullptr) { // Damn. Create the queue of that guy
            otherqueue = new std::queue<simgrid::xbt::ReplayAction*>();
            xbt_dict_set(xbt_action_queues, evtname.c_str(), otherqueue, nullptr);
          }
          otherqueue->push(action);
        }
      }
    }
    // end of file reached while searching in vain for more work
  } else {
    // Get something from my queue and return it
    action = myqueue->front();
    myqueue->pop();
    return action;
  }

// All my actions in the file are done and either I or a colleague closed the file. Let's cleanup before leaving.
todo_done:
  if (myqueue != nullptr) {
    delete myqueue;
    xbt_dict_remove(xbt_action_queues, name);
  }
  return nullptr;
}
