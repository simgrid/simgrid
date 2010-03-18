#ifndef RB_SD_WORKSTATION_H
#define RB_SD_WORKSTATION_H

#include <ruby.h>
#include <simdag/simdag.h>

// free
static void SD_workstation_free(SD_workstation_t);

// Workstation List
static VALUE rb_SD_workstation_list(VALUE Class);

// Workstation number
static VALUE rb_SD_workstation_number(VALUE Class);

// Workstation name
static VALUE rb_SD_workstation_name(VALUE Class,VALUE workstation);

#endif