/* Copyright (c) 2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/ex.hpp>
#include "src/internal_config.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/file.h"
#include "xbt/replay.h"

#include <boost/algorithm/string.hpp>
#include <ctype.h>
#include <errno.h>
#include <fstream>
#include <wchar.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(replay,xbt,"Replay trace reader");

namespace simgrid {
namespace xbt {

class ReplayReader {
  std::ifstream* fs;
  std::string line;

public:
  char* filename_;
  int linenum = 0;

  ReplayReader(const char* filename)
  {
    filename_ = xbt_strdup(filename);
    fs        = new std::ifstream(filename, std::ifstream::in);
  }
  ~ReplayReader()
  {
    free(filename_);
    delete fs;
  }
  bool get(std::vector<std::string>* evt);
};

bool ReplayReader::get(std::vector<std::string>* evt)
{
  std::getline(*fs, line);
  boost::trim(line);
  XBT_DEBUG("got from trace: %s", line.c_str());
  linenum++;

  if (line.length() > 0 && line.find("#") == std::string::npos) {
    std::vector<std::string> res;
    boost::split(*evt, line, boost::is_any_of(" \t"), boost::token_compress_on);
    return !fs->eof();
  } else {
    if (fs->eof())
      return false;
    else
      return this->get(evt);
  }
}
}
}

typedef struct s_replay_reader {
  FILE *fp;
  char *line;
  size_t line_len;
  char *position; /* stable storage */
  char *filename; 
  int linenum;
} s_xbt_replay_reader_t;

FILE *xbt_action_fp;

xbt_dict_t xbt_action_funs = nullptr;
xbt_dict_t xbt_action_queues = nullptr;

static char *action_line = nullptr;
static size_t action_len = 0;

int is_replay_active = 0 ;

static char **action_get_action(char *name);

static char *str_tolower (const char *str)
{
  char *ret = xbt_strdup (str);
  int n     = strlen(ret);
  for (int i = 0; i < n; i++)
    ret[i] = tolower (str[i]);
  return ret;
}

int _xbt_replay_is_active(){
  return is_replay_active;
}

/**
 * \ingroup XBT_replay
 * \brief Registers a function to handle a kind of action
 *
 * Registers a function to handle a kind of action
 * This table is then used by \ref xbt_replay_action_runner
 *
 * The argument of the function is the line describing the action, splitted on spaces with xbt_str_split_quoted()
 *
 * \param action_name the reference name of the action.
 * \param function prototype given by the type: void...(xbt_dynar_t action)
 */
void xbt_replay_action_register(const char *action_name, action_fun function)
{
  if (xbt_action_funs == nullptr) // If the user registers a function before the start
    _xbt_replay_action_init();

  char* lowername = str_tolower (action_name);
  xbt_dict_set(xbt_action_funs, lowername, (void*) function, nullptr);
  xbt_free(lowername);
}

/** @brief Initializes the replay mechanism, and returns true if (and only if) it was necessary
 *
 * It returns false if it was already done by another process.
 */
int _xbt_replay_action_init()
{
  if (xbt_action_funs)
    return 0;
  is_replay_active = 1;
  xbt_action_funs = xbt_dict_new_homogeneous(nullptr);
  xbt_action_queues = xbt_dict_new_homogeneous(nullptr);
  return 1;
}

void _xbt_replay_action_exit()
{
  xbt_dict_free(&xbt_action_queues);
  xbt_dict_free(&xbt_action_funs);
  free(action_line);
  xbt_action_queues = nullptr;
  xbt_action_funs = nullptr;
  action_line = nullptr;
}

/**
 * \ingroup XBT_replay
 * \brief function used internally to actually run the replay

 * \param argc argc .
 * \param argv argv
 */
int xbt_replay_action_runner(int argc, char *argv[])
{
  if (xbt_action_fp) {              // A unique trace file
    while (true) {
      char **evt = action_get_action(argv[0]);
      if (evt == nullptr)
        break;

      char* lowername = str_tolower (evt[1]);
      action_fun function = (action_fun)xbt_dict_get(xbt_action_funs, lowername);
      xbt_free(lowername);
      try {
        function((const char **)evt);
      }
      catch(xbt_ex& e) {
        xbt_die("Replay error :\n %s", e.what());
      }
      for (int i=0;evt[i]!= nullptr;i++)
        free(evt[i]);
      free(evt);
    }
  } else {                      // Should have got my trace file in argument
    std::vector<std::string>* evt = new std::vector<std::string>();
    xbt_assert(argc >= 2,
                "No '%s' agent function provided, no simulation-wide trace file provided, "
                "and no process-wide trace file provided in deployment file. Aborting.", argv[0]
        );
    simgrid::xbt::ReplayReader* reader = new simgrid::xbt::ReplayReader(argv[1]);
    while (reader->get(evt)) {
      if (evt->at(0).compare(argv[0]) == 0) {
        std::string lowername = evt->at(1);
        boost::algorithm::to_lower(lowername);
        char** args = new char*[evt->size() + 1];
        int i       = 0;
        for (auto arg : *evt) {
          args[i] = xbt_strdup(arg.c_str());
          i++;
        }
        args[i]             = nullptr;
        action_fun function = (action_fun)xbt_dict_get(xbt_action_funs, lowername.c_str());
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

static char **action_get_action(char *name)
{
  xbt_dynar_t evt = nullptr;
  char *evtname = nullptr;

  xbt_dynar_t myqueue = (xbt_dynar_t) xbt_dict_get_or_null(xbt_action_queues, name);
  if (myqueue == nullptr || xbt_dynar_is_empty(myqueue)) {      // nothing stored for me. Read the file further
    if (xbt_action_fp == nullptr) {    // File closed now. There's nothing more to read. I'm out of here
      goto todo_done;
    }
    // Read lines until I reach something for me (which breaks in loop body)
    // or end of file reached
    while (xbt_getline(&action_line, &action_len, xbt_action_fp) != -1) {
      // cleanup and split the string I just read
      char *comment = strchr(action_line, '#');
      if (comment != nullptr)
        *comment = '\0';
      xbt_str_trim(action_line, nullptr);
      if (action_line[0] == '\0')
        continue;
      /* we cannot split in place here because we parse&store several lines for
       * the colleagues... */
      evt = xbt_str_split_quoted(action_line);

      // if it's for me, I'm done
      evtname = xbt_dynar_get_as(evt, 0, char *);
      if (!strcasecmp(name, evtname)) {
        return (char**) xbt_dynar_to_array(evt);
      } else {
        // Else, I have to store it for the relevant colleague
        xbt_dynar_t otherqueue =
            (xbt_dynar_t) xbt_dict_get_or_null(xbt_action_queues, evtname);
        if (otherqueue == nullptr) {       // Damn. Create the queue of that guy
          otherqueue = xbt_dynar_new(sizeof(xbt_dynar_t), xbt_dynar_free_voidp);
          xbt_dict_set(xbt_action_queues, evtname, otherqueue, nullptr);
        }
        xbt_dynar_push(otherqueue, &evt);
      }
    }
    // end of file reached while searching in vain for more work
  } else {
    // Get something from my queue and return it
    xbt_dynar_shift(myqueue, &evt);
    return (char**) xbt_dynar_to_array(evt);
  }

  // I did all my actions for me in the file (either I closed the file, or a colleague did)
  // Let's cleanup before leaving
todo_done:
  if (myqueue != nullptr) {
    xbt_dynar_free(&myqueue);
    xbt_dict_remove(xbt_action_queues, name);
  }
  return nullptr;
}
