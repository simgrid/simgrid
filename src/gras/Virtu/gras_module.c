/* module - module handling, along with module dependencies and local state */

/* Copyright (C) 2006 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt.h"
#include "gras/module.h"
#include "virtu_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_modules, gras,
                                "Module and moddata handling");

/* IMPLEMENTATION NOTE

The goal of this module is quite difficult. We want to have several modules,
each of them having globals which have to be initialized in any unix
process. Let's call them world-wide globals. An example may be the messages
dictionnary, which is world-wide to detect definition discrepencies when
running in SG.

And then, the module may want to have some globals specific to a gras process,
just like userdata stuff allows to have per-process globals. Let's call them
process-wide globals. An example may be the list of attached callbacks.

That is why we have 4 functions per module: the join function is called by any
process, and is in charge of creating the process-wide globals. If it is the
first time a process uses the module, it also calls the init function, in
charge of initializing the world-wide globals. We have the symmetric functions
leave, called by any process not using the module anymore, and exit, in charge
of freeing the world-wide globals when no process use it anymore.

One can see this as a factory of factories. the init function is a factory
creating the module, which contains a factory (the join function) able to
create process-wide globals. The fact that indeed the calling sequence goes
from join to init and not the other side is just an implementation bias ;)

Then again, we want these functionalities to be quick. We want to access
the process-wide globals by providing their rank in a dynar, we don't want to
search them in a dictionary. This is especially true in the module
implementation, where each functions is likely to require them to work. The
position could be a stored in a global variable only visible from the module
implementation.

The need for an array in which to store the globals does not hold for
world-wide globals: only one instance of them can exist in the same unix naming
space. Thus, there is no need to put them in an array, we can actually declare
them as regular C globals.

The next trick comes from the fact that the user may (and will) mess up and not
get all modules initialized in the same order in every process. So, the rank of
module data cannot be the order in which a process called the join
function. This has to be a world-wide global instead.

And finally, if we want to get the ability to destroy modules (one day, they
will load their code in as plugin), we want to give the module handling library
(ie, this file) to modify the variable containing the rank of module in the
tables. This is why we have the ID field in the module structure: it points
exactly to the implementation side global.

Yeah, I know. All this is not that clear. But at least, writing this helped me
to design that crap ;)

*/

typedef struct s_gras_module {
  XBT_SET_HEADERS;

  unsigned int datasize;
  int refcount;                 /* Number of processes using this module */
  /* FIXME: we should keep a count of references within a given process to
     allow modules initializing other modules while tracking dependencies
     properly and leave() only when needed. This would allow dynamic module
     loading/unloading */

  int *p_id;                    /* where the module stores the libdata ID (a global somewhere), to tweak it on need */
  void_f_void_t init_f;         /* First time the module is referenced. */
  void_f_void_t exit_f;         /* When last process referencing it stops doing so. */
  void_f_pvoid_t join_f;        /* Called by each process in initialization phase (init_f called once for all processes) */
  void_f_pvoid_t leave_f;       /* Called by each process in finalization phase. Should free moddata passed */
} s_gras_module_t, *gras_module_t;

static xbt_set_t _gras_modules = NULL;  /* content: s_gras_module_t */

static void gras_module_freep(void *p)
{
  free(((gras_module_t) p)->name);
  free(p);
}


/**
 * @brief Declaring a new GRAS module
 * @param name: name of the module, of course (beware of dupplicates!)
 * @param datasize: the size of your data, ie of the state this module has on each process
 * @param ID: address of a global you use as parameter to gras_module_data_by_id
 * @param init_f: function called the first time a module gets by a process of the naming space.
 *                A classical use is to declare some messages the module uses, as well as the initialization
 *                of module constants (accross processes boundaries in SG).
 * @param exit_f: function called when the last process of this naming space unref this module.
 * @param join_f: function called each time a process references the module.
 *                It is passed the moddata already malloced, and should initialize the fields as it wants.
 *                It can also attach some callbacks to the module messages.
 * @param leave_f: function called each time a process unrefs the module.
 *                 It is passed the moddata, and should free any memory allocated by init_f.
 *                 It should alse disconnect any attached callbacks.
 */

void gras_module_add(const char *name, unsigned int datasize, int *ID,
                     void_f_void_t init_f, void_f_void_t exit_f,
                     void_f_pvoid_t join_f, void_f_pvoid_t leave_f)
{
  gras_module_t mod = NULL;
  xbt_ex_t e;
  volatile int found = 0;

  if (!_gras_modules)
    _gras_modules = xbt_set_new();

  TRY {
    mod = (gras_module_t) xbt_set_get_by_name(_gras_modules, name);
    found = 1;
  }
  CATCH(e) {
    if (e.category != not_found_error)
      RETHROW;
    xbt_ex_free(e);
  }

  if (found) {
    xbt_assert1(mod->init_f == init_f,
                "Module %s reregistered with a different init_f!", name);
    xbt_assert1(mod->exit_f == exit_f,
                "Module %s reregistered with a different exit_f!", name);
    xbt_assert1(mod->join_f == join_f,
                "Module %s reregistered with a different join_f!", name);
    xbt_assert1(mod->leave_f == leave_f,
                "Module %s reregistered with a different leave_f!", name);
    xbt_assert1(mod->datasize == datasize,
                "Module %s reregistered with a different datasize!", name);
    xbt_assert1(mod->p_id == ID,
                "Module %s reregistered with a different p_id field!", name);

    DEBUG1("Module %s already registered. Ignoring re-registration", name);
    return;
  }

  VERB1("Register module %s", name);
  mod = xbt_new(s_gras_module_t, 1);
  mod->name = xbt_strdup(name);
  mod->name_len = strlen(name);

  mod->datasize = datasize;
  mod->p_id = ID;
  mod->init_f = init_f;
  mod->exit_f = exit_f;
  mod->join_f = join_f;
  mod->leave_f = leave_f;
  mod->refcount = 0;

  *mod->p_id = xbt_set_length(_gras_modules);

  xbt_set_add(_gras_modules, (void *) mod, gras_module_freep);
}

/* shutdown the module mechanism (world-wide cleanups) */
void gras_moddata_exit(void)
{
  xbt_set_free(&_gras_modules);
}

/* frees the moddata on this host (process-wide cleanups) */
void gras_moddata_leave(void)
{
  gras_procdata_t *pd = gras_procdata_get();

  xbt_dynar_free(&pd->moddata);
}

/* Removes & frees a given moddata from the current host */
static void moddata_freep(void *p)
{
  gras_procdata_t *pd = gras_procdata_get();
  int id = xbt_dynar_search(pd->moddata, p);
  gras_module_t mod = (gras_module_t) xbt_set_get_by_id(_gras_modules, id);

  (*mod->leave_f) (gras_moddata_by_id(id));
}

void gras_module_join(const char *name)
{
  gras_procdata_t *pd;
  void *moddata;
  gras_module_t mod =
    (gras_module_t) xbt_set_get_by_name(_gras_modules, name);

  VERB2("Join to module %s (%p)", name, mod);

  /* NEW */
  if (mod->refcount == 0) {
    VERB1("Init module %s", name);
    mod->name = xbt_strdup(name);

    (*mod->init_f) ();
  } else {
    DEBUG3("Module %s already inited. Refcount=%d ID=%d",
           mod->name, mod->refcount, *(mod->p_id));
  }
  mod->refcount++;

  /* JOIN */
  pd = gras_procdata_get();

  if (!pd->moddata)             /* Damn. I must be the first module on this process. Scary ;) */
    pd->moddata = xbt_dynar_new(sizeof(gras_module_t), &moddata_freep);

  moddata = xbt_malloc(mod->datasize);

  xbt_dynar_set(pd->moddata, *(mod->p_id), &moddata);

  (*mod->join_f) (moddata);

  DEBUG2("Module %s joined successfully (ID=%d)", name, *(mod->p_id));
}

void gras_module_leave(const char *name)
{
  void *moddata;
  gras_module_t mod =
    (gras_module_t) xbt_set_get_by_name(_gras_modules, name);

  VERB1("Leave module %s", name);

  /* LEAVE */
  moddata = gras_moddata_by_id(*(mod->p_id));
  (*mod->leave_f) (moddata);

  /* EXIT */
  mod->refcount--;
  if (!mod->refcount) {
    VERB1("Exit module %s", name);

    (*mod->exit_f) ();

    /* Don't remove the module for real, sets don't allow to

       free(mod->name);
       free(mod);
     */
  }
}


void *gras_moddata_by_id(unsigned int ID)
{
  gras_procdata_t *pd = gras_procdata_get();
  void *p;

  xbt_dynar_get_cpy(pd->moddata, ID, &p);
  return p;
}
