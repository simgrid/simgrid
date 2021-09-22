.. _xbt:

The XBT toolbox
###############

XBT is not an interface to describe your application, but rather a toolbox of features that are used everywhere in SimGrid. The code described in this page is not specific to any
of the existing interfaces, and it's used in all of them.

.. _logging_prog:

Logging API
***********

As introduced in :ref:`outcome_logs`, the SimGrid logging mechanism allows to configure at runtime the messages that should be displayed and those that should be omitted. Each
message produced in the code is given a category (denoting its topic) and a priority. Then at runtime, each category is given a threshold (only messages of priority higher than
that threshold are displayed), a layout (deciding how the messages in this category are formatted), and an appender (deciding what to do with the message: either print on stderr or
to a file).

This section documents the provided API. To see how to configure these features at runtime, please refer to :ref:`logging_config`.

Declaring categories
====================

Typically, there will be a category for each module of the implementation, so that users can independently control the logging for each module.
Refer to the :ref:`logging_categories` section for a list of all existing categories in SimGrid.

.. c:macro:: XBT_LOG_NEW_CATEGORY(category, description)

   Creates a new category that is not within any existing categories. It will be located right below the ``root`` category.
   ``category`` must be a valid identifier (such as ``mycat``) with no quote or anything. It should also be unique over the whole binary.
   ``description`` must be a string, between quotes.

.. c:macro:: XBT_LOG_NEW_SUBCATEGORY(category, parent_category, description)

   Creates a new category under the provided ``parent_category``.

.. c:macro:: XBT_LOG_NEW_DEFAULT_CATEGORY(category, description)

   Similar to :c:macro:`XBT_LOG_NEW_CATEGORY`, and the created category is the default one in the current source file.

.. c:macro:: XBT_LOG_NEW_DEFAULT_SUBCATEGORY(category, parent_category, description)

   Similar to :c:macro:`XBT_LOG_NEW_SUBCATEGORY`, and the created category is the default one in the current source file.

.. c:macro:: XBT_LOG_EXTERNAL_CATEGORY(category)

   Make an external category (i.e., a category declared in another source file) visible from this source file. 
   In each source file, at most one one category can be the default one.

.. c:macro:: XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(category)

   Use an external category as default category in this source file.

Logging messages
================

Default category
----------------

.. c:macro:: XBT_CRITICAL(format_string, parameters...)

   Report a fatal error to the default category.

.. c:macro:: XBT_ERROR(format_string, parameters...)

   Report an error to the default category.

.. c:macro:: XBT_WARN(format_string, parameters...)

   Report a warning or an important information to the default category.

.. c:macro:: XBT_INFO(format_string, parameters...)

   Report an information of regular importance to the default category.

.. c:macro:: XBT_VERB(format_string, parameters...)

   Report a verbose information to the default category.

.. c:macro:: XBT_DEBUG(format_string, parameters...)

   Report a debug-only information to the default category.

For each of the logging macros, the first parameter must be a printf-like format string, and the subsequent parameters must match this format. If you compile with the -Wall option,
the compiler will warn you for unmatched arguments, such as passing a pointer while the format string expects an integer. Using this option is usually a good idea.

Here is an example: ``XBT_WARN("Values are: %d and '%s'", 5, "oops");``

.. c:macro:: XBT_IN(format_string, parameters...)

   Report that the execution flow enters a given function (which name is displayed automatically).

.. c:macro:: XBT_OUT(format_string, parameters...)

   Report that the execution flow exits a given function (which name is displayed automatically).

.. c:macro:: XBT_HERE(format_string, parameters...)

   Report that the execution flow reaches a given location.

Specific category
-----------------

.. c:macro:: XBT_CCRITICAL(category, format_string, parameters...)

   Report a fatal error to the specified ``category``.

.. c:macro:: XBT_CERROR(category, format_string, parameters...)

   Report an error to the specified ``category``.

.. c:macro:: XBT_CWARN(category, format_string, parameters...)

   Report a warning or an important information to the specified ``category``.

.. c:macro:: XBT_CINFO(category, format_string, parameters...)

   Report an information of regular importance to the specified ``category``.

.. c:macro:: XBT_CVERB(category, format_string, parameters...)

   Report a verbose information to the specified ``category``.

.. c:macro:: XBT_CDEBUG(category, format_string, parameters...)

   Report a debug-only information to the specified ``category``.

Of course, the specified category must be visible from this source file, either because it was created there (e.g. with :c:macro:`XBT_LOG_NEW_CATEGORY`) or because it was made
visible with :c:macro:`XBT_LOG_EXTERNAL_CATEGORY`.

Other functions
===============

.. c:macro:: XBT_LOG_ISENABLED(category, priority)

   Returns true if that category displays the messages of that priority. It's useful to compute a value that is used only in the logging, such as the textual representation of a
   non-trivial object.

   The specified priority must be one of ``xbt_log_priority_trace``, ``xbt_log_priority_debug``, ``xbt_log_priority_verbose``, ``xbt_log_priority_info``,
   ``xbt_log_priority_warning``, ``xbt_log_priority_error`` or ``xbt_log_priority_critical``.

.. c:function:: void xbt_log_control_set(const char* setting)

   Sets the provided ``setting`` as if it was passed in a ``--log`` command-line parameter.

You should not use any of the macros which name starts with '_'.

.. include:: ../build/log_categories.rst

Full example
============

.. code-block:: C

   #include "xbt/log.h"

   /* create a category and a default subcategory */
   XBT_LOG_NEW_CATEGORY(VSS);
   XBT_LOG_NEW_DEFAULT_SUBCATEGORY(SA, VSS);

   int main() {
       /* Now set the parent's priority.  (the string would typically be a runtime option) */
       xbt_log_control_set("SA.thresh:info");

       /* This request is enabled, because WARNING >= INFO. */
       XBT_CWARN(VSS, "Low fuel level.");

       /* This request is disabled, because DEBUG < INFO. */
       XBT_CDEBUG(VSS, "Starting search for nearest gas station.");

       /* The default category SA inherits its priority from VSS. Thus,
          the following request is enabled because INFO >= INFO.  */
       XBT_INFO("Located nearest gas station.");

       /* This request is disabled, because DEBUG < INFO. */
       XBT_DEBUG("Exiting gas station search");
   }

Performance concern
===================

This module is highly optimized. Messages that will not be displayed are not even built. For example, using ``XBT_DEBUG`` in a category that turns debug messages off only costs a
single integer comparison at runtime, and the parameters are not even evaluated.

You can even specify a compile-time threshold that will completely remove every logging below the specified priority. Passing ``-DNDEBUG`` to cmake disables every logging of
priority below INFO while ``-DNLOG`` removes any logging at compile time. Note that using this feature may hinder the stability of SimGrid, as we consider the logs to be fast
enough to not thoughtfully test the case where they are removed at compile time.

Dynamic arrays
**************

As SimGrid used to be written in pure C, it used to rely on custom data containers such as dynamic arrays and dictionnaries. Nowadays, the standard library of
C++ is used internally, but some part of the interface still rely on the old containers, that are thus still available. 

.. warning:: 

   You should probably not start a new project using these data structures,  as we will :ref:`deprecate them from SimGrid <deprecation_policy>` 
   as soon as possible. Better implementations exist out there anyway, in particular if you're not writting pure C code.

.. doxygentypedef:: xbt_dynar_t
.. doxygentypedef:: const_xbt_dynar_t

Creation and destruction
========================

.. doxygenfunction:: xbt_dynar_new
.. doxygenfunction:: xbt_dynar_free
.. doxygenfunction:: xbt_dynar_free_container
.. doxygenfunction:: xbt_dynar_shrink

Dynars as regular arrays
========================
   
.. doxygenfunction:: xbt_dynar_get_cpy
.. doxygenfunction:: xbt_dynar_insert_at
.. doxygenfunction:: xbt_dynar_remove_at
.. doxygenfunction:: xbt_dynar_member
.. doxygenfunction:: xbt_dynar_sort

Dynar size
==========

.. doxygenfunction:: xbt_dynar_is_empty
.. doxygenfunction:: xbt_dynar_length
.. doxygenfunction:: xbt_dynar_reset
   
Perl-like interface
===================

.. doxygenfunction:: xbt_dynar_push
.. doxygenfunction:: xbt_dynar_pop
.. doxygenfunction:: xbt_dynar_unshift
.. doxygenfunction:: xbt_dynar_shift
.. doxygenfunction:: xbt_dynar_map

Direct content manipulation
===========================

Those functions do not retrieve the content, but only their address.

.. doxygenfunction:: xbt_dynar_set_at_ptr
.. doxygenfunction:: xbt_dynar_get_ptr
.. doxygenfunction:: xbt_dynar_insert_at_ptr
.. doxygenfunction:: xbt_dynar_push_ptr
.. doxygenfunction:: xbt_dynar_pop_ptr

Dynars of scalars
=================

While the other functions use a memcpy to retrieve the content into the user provided area, those ones use a
regular affectation. It only works for scalar values, but should be a little faster.

.. doxygendefine:: xbt_dynar_get_as
.. doxygendefine:: xbt_dynar_set_as

.. doxygendefine:: xbt_dynar_getlast_as
.. doxygendefine:: xbt_dynar_getfirst_as
.. doxygendefine:: xbt_dynar_push_as
.. doxygendefine:: xbt_dynar_pop_as

Iterating over Dynars
=====================

.. doxygendefine:: xbt_dynar_foreach

Example with scalar values
==========================

.. code-block:: c

   xbt_dynar_t d = xbt_dynar_new(sizeof(int), nullptr);

   /* 1. Populate the dynar */
   xbt_dynar_t d = xbt_dynar_new(sizeof(int), nullptr);
   for (int cpt = 0; cpt < NB_ELEM; cpt++) {
     xbt_dynar_push_as(d, int, cpt);     /* This is faster (and possible only with scalars) */
     /* xbt_dynar_push(d,&cpt);       This would also work */
     xbt_test_log("Push %d, length=%lu", cpt, xbt_dynar_length(d));
   }

   /* 2. Traverse manually the dynar */
   for (cursor = 0; cursor < NB_ELEM; cursor++) {
     int* iptr = (int*)xbt_dynar_get_ptr(d, cursor);
   /* 1. Populate further the dynar */
   for (int cpt = 0; cpt < NB_ELEM; cpt++) {
     xbt_dynar_insert_at(d, cpt, &cpt);
     xbt_test_log("Push %d, length=%lu", cpt, xbt_dynar_length(d));
   }

   /* 3. Traverse the dynar */
   int cpt;
   xbt_dynar_foreach(d, cursor, cpt) {
     xbt_test_assert(cursor == (unsigned int) cpt, "The retrieved value is not the same than the injected one (%u!=%d)", cursor, cpt);
   }

   /* 4. Reset the values */
   for (int i = 0; i < NB_ELEM; i++)
     *(int*)xbt_dynar_get_ptr(d, i) = i;

   /* 5. Shift all the values */
   for (int i = 0; i < NB_ELEM; i++) {
      int val;
      xbt_dynar_shift(d, &val);
   }
   // the dynar is empty after shifting all values

   /* 5. Free the resources */
   xbt_dynar_free(&d);

Example with pointed values
===========================

.. code-block:: c

   xbt_dynar_t d = xbt_dynar_new(sizeof(char*), &xbt_free_ref);

   /// Push/shift example 
   for (int i = 0; i < NB_ELEM; i++) {
     char * val = xbt_strdup("hello");
     xbt_dynar_push(d, &val);
   }
   for (int i = 0; i < NB_ELEM; i++) {
     char *val;
     xbt_dynar_shift(d, &val);
     REQUIRE("hello" == val);
     xbt_free(val);
   }
   xbt_dynar_free(&d);

   /// Unshift, traverse and pop example
   d = xbt_dynar_new(sizeof(char**), &xbt_free_ref);
   for (int i = 0; i < NB_ELEM; i++) {
     std::string val = std::to_string(i);
     s1              = xbt_strdup(val.c_str());
     xbt_dynar_unshift(d, &s1);
   }
   /* 2. Traverse the dynar with the macro */
   xbt_dynar_foreach (d, iter, s1) {
     std::string val = std::to_string(NB_ELEM - iter - 1);
     REQUIRE(s1 == val); // The retrieved value is not the same than the injected one
   }
   /* 3. Traverse the dynar with the macro */
   for (int i = 0; i < NB_ELEM; i++) {
     std::string val = std::to_string(i);
     xbt_dynar_pop(d, &s2);
     REQUIRE(s2 == val); // The retrieved value is not the same than the injected one
     xbt_free(s2);
   }
   /* 4. Free the resources */
   xbt_dynar_free(&d);
