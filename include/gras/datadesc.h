/* $Id$ */

/* gras/datadesc.h - Describing the data you want to exchange               */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_DATADESC_H
#define GRAS_DATADESC_H

#include "xbt/misc.h" /* BEGIN_DECL */

BEGIN_DECL()

/** @addtogroup GRAS_dd Data description
 *  @brief Describing data to be exchanged (Communication facility)
 *
 * @section Overview
 *
 * Since GRAS takes care of potential representation conversion when the platform is heterogeneous, 
 * any data which transits on the network must be described beforehand.
 * 
 * There is several possible interfaces for this, ranging from the really completely automatic parsing to 
 * completely manual. Let's study each of them from the simplest to the more advanced.
 * 
 * \warning At least, I would like to present those sections in the right order, but doxygen prevents me 
 * from doing so. There is a weird bug I fail to circumvent here. The right order is naturally:
 *   -# basic operations
 *   -# Automatic parsing
 *   -# Simple manual definitions
 *   -# Callback Persistant State: Simple push/pop mecanism
 *   -# Callback Persistant State: Full featured mecanism
 */
/*@{*/

/** @name 1. basic operations
 *
 * If you only want to send pre-existing types, simply retrieve the pre-defined description with 
 * the \ref gras_datadesc_by_name function. Existing types entail:
 *  - char (both signed and unsigned)
 *  - int (short, regular, long and long long, both signed and unsigned)
 *  - float and double
 *  - string (which is indeed a reference to a dynamically sized array of char, strlen being used to retrive the size)
 * 
 * Example:\verbatim gras_datadesc_type_t i = gras_datadesc_by_name("int");
 gras_datadesc_type_t uc = gras_datadesc_by_name("unsigned char");
 gras_datadesc_type_t str = gras_datadesc_by_name("string");\endverbatim
 */
/* @{ */
  
/** @brief Opaque type describing a type description. */
typedef struct s_gras_datadesc_type *gras_datadesc_type_t;

/** \brief Search a type description from its name */
gras_datadesc_type_t gras_datadesc_by_name(const char *name);

/* @} */
    
/** @name 2. Automatic parsing
 * 
 *  If you need to declare a new datatype, this is the simplest way to describe it to GRAS. Simply
 *  enclose its type definition  into a \ref GRAS_DEFINE_TYPE macro call, and you're set. Here is 
 *  an type declaration  example: \verbatim GRAS_DEFINE_TYPE(mytype,struct mytype {
   int myfirstfield;
   char mysecondfield;
 });\endverbatim
 *  The type is then both copied verbatim into your source file and stored for further parsing. This allows
 *  you to let GRAS parse the exact version you are actually using in your program.
 *  You can then retrieve the corresponding type description with \ref gras_datadesc_by_symbol.
 *  Don't worry too much for the performances, the type is only parsed once and a binary representation 
 *  is stored and used in any subsequent calls.
 * 
 *  If your structure contains any pointer, you have to explain GRAS the size of the pointed array. This
 *  can be 1 in the case of simple references, or more in the case of regular arrays. For that, use the 
 *  \ref GRAS_ANNOTE macro within the type declaration you are passing to \ref GRAS_DEFINE_TYPE. This macro
 *  rewrites itself to nothing in the declaration (so they won't pollute the type definition copied verbatim
 *  into your code), and give some information to GRAS about your pointer. 
 
 *  GRAS_ANNOTE takes two arguments being the key name and the key value. For now, the only accepted key name 
 *  is "size", to specify the length of the pointed array. It can either be the string "1" (without the quote)
 *  or the name of another field of the structure.
 *  
 *  Here is an example:\verbatim GRAS_DEFINE_TYPE(s_clause,
  struct s_array {
    int length;
    int *data GRAS_ANNOTE(size,length);
    struct s_array *father GRAS_ANNOTE(size,1);
 }
;)\endverbatim
 * It specifies that the structure s_array contains two fields, and that the size of the array pointed 
 * by \a data is the \a length field, and that the \a father field is a simple reference.
 * 
 * If you cannot express your datadescs with this mecanism, you'll have to use the more advanced 
 * (and somehow complex) one described below.
 * 
 *  \warning Since GRAS_DEFINE_TYPE is a macro, you shouldn't  put any comma in your type definition 
 *  (comma separates macro args). 
 * 
 *  For example, change \verbatim int a, b;\endverbatim to \verbatim int a;
 int b:\endverbatim
 */
/** @{ */

 
/**   @brief Automatically parse C code
 *    @hideinitializer
 */
#define GRAS_DEFINE_TYPE(name,def) \
  static const char * _gras_this_type_symbol_does_not_exist__##name=#def; def
 
/** @brief Retrieve a datadesc which was previously parsed 
 *  @hideinitializer
 */
#define gras_datadesc_by_symbol(name)  \
  (gras_datadesc_by_name(#name) ?      \
   gras_datadesc_by_name(#name) :      \
     gras_datadesc_parse(#name,        \
			 _gras_this_type_symbol_does_not_exist__##name) \
  )

/** @def GRAS_ANNOTE
 *  @brief Add an annotation to a type to be automatically parsed
 */
#define GRAS_ANNOTE(key,val)

/*@}*/

gras_datadesc_type_t 
gras_datadesc_parse(const char *name, const char *C_statement);

/** @name 3. Simple manual definitions
 * 
 * Here are the functions to use if you want to declare your description manually. 
 * The function names should be self-explanatory in most cases.
 * 
 * You can add callbacks to the datatypes doing any kind of action you may want. Usually, 
 * pre-send callbacks are used to prepare the type expedition while post-receive callbacks 
 * are used to fix any issue after the receive.
 * 
 * If your types are dynamic, you'll need to add some extra callback. For example, there is a
 * specific callback for the string type which is in charge of computing the length of the char
 * array. This is done with the cbps mecanism, explained in next section.
 * 
 * If your types may contain pointer cycle, you must specify it to GRAS using the @ref gras_datadesc_cycle_set. 
 * 
 * Example:\verbatim
 typedef struct {
   unsigned char c1;
   unsigned long int l1;
   unsigned char c2;
   unsigned long int l2;
 } mystruct;
 [...]
  my_type=gras_datadesc_struct("mystruct");
  gras_datadesc_struct_append(my_type,"c1", gras_datadesc_by_name("unsigned char"));
  gras_datadesc_struct_append(my_type,"l1", gras_datadesc_by_name("unsigned long"));
  gras_datadesc_struct_append(my_type,"c2", gras_datadesc_by_name("unsigned char"));
  gras_datadesc_struct_append(my_type,"l2", gras_datadesc_by_name("unsigned long int"));
  gras_datadesc_struct_close(my_type);

  my_type=gras_datadesc_ref("mystruct*", gras_datadesc_by_name("mystruct"));
  
  [Use my_type to send pointers to mystruct data]\endverbatim
 */
/*@{*/


/** \brief Opaque type describing a type description callback persistant state. */
typedef struct s_gras_cbps *gras_cbps_t;

/* callbacks prototypes */
/** \brief Prototype of type callbacks returning nothing. */
typedef void (*gras_datadesc_type_cb_void_t)(gras_cbps_t vars, void *data);
/** \brief Prototype of type callbacks returning an int. */
typedef int (*gras_datadesc_type_cb_int_t)(gras_cbps_t vars, void *data);
/** \brief Prototype of type callbacks selecting a type. */
typedef gras_datadesc_type_t (*gras_datadesc_selector_t)(gras_cbps_t vars, void *data);


/******************************************
 **** Declare datadescription yourself ****
 ******************************************/

gras_datadesc_type_t gras_datadesc_struct(const char *name);
void gras_datadesc_struct_append(gras_datadesc_type_t  struct_type,
				 const char           *name,
				 gras_datadesc_type_t  field_type);
void gras_datadesc_struct_close(gras_datadesc_type_t   struct_type);


gras_datadesc_type_t gras_datadesc_union(const char                 *name,
					 gras_datadesc_type_cb_int_t selector);
void gras_datadesc_union_append(gras_datadesc_type_t   union_type,
				const char            *name,
				gras_datadesc_type_t   field_type);
void gras_datadesc_union_close(gras_datadesc_type_t    union_type);


gras_datadesc_type_t 
  gras_datadesc_ref(const char          *name,
		    gras_datadesc_type_t referenced_type);
gras_datadesc_type_t 
  gras_datadesc_ref_generic(const char              *name,
			    gras_datadesc_selector_t selector);

gras_datadesc_type_t 
  gras_datadesc_array_fixed(const char          *name,
			    gras_datadesc_type_t element_type,
			    long int             fixed_size);
gras_datadesc_type_t 
  gras_datadesc_array_dyn(const char                 *name,
			  gras_datadesc_type_t        element_type,
			  gras_datadesc_type_cb_int_t dynamic_size);
gras_datadesc_type_t 
  gras_datadesc_ref_pop_arr(gras_datadesc_type_t  element_type);

/*********************************
 * Change stuff within datadescs *
 *********************************/

/** \brief Specify that this type may contain cycles */
void gras_datadesc_cycle_set(gras_datadesc_type_t type);
/** \brief Specify that this type do not contain any cycles (default) */
void gras_datadesc_cycle_unset(gras_datadesc_type_t type);
/** \brief Add a pre-send callback to this datadesc. */
void gras_datadesc_cb_send (gras_datadesc_type_t         type,
			    gras_datadesc_type_cb_void_t pre);
/** \brief Add a post-receive callback to this datadesc.*/
void gras_datadesc_cb_recv(gras_datadesc_type_t          type,
			   gras_datadesc_type_cb_void_t  post);
/** \brief Add a pre-send callback to the given field of the datadesc */
void gras_datadesc_cb_field_send (gras_datadesc_type_t   type,
				  const char            *field_name,
				  gras_datadesc_type_cb_void_t  pre);
/** \brief Add a post-receive callback to the given field of the datadesc */
void gras_datadesc_cb_field_recv(gras_datadesc_type_t    type,
				 const char             *field_name,
				 gras_datadesc_type_cb_void_t  post);
/** \brief Add a pre-send callback to the given field resulting in its value to be pushed */
void gras_datadesc_cb_field_push (gras_datadesc_type_t   type,
				  const char            *field_name);

/******************************
 * Get stuff within datadescs *
 ******************************/
/** \brief Returns the name of a datadescription */
char * gras_datadesc_get_name(gras_datadesc_type_t ddt);
/** \brief Returns the identifier of a datadescription */
int gras_datadesc_get_id(gras_datadesc_type_t ddt);

/*@}*/

/** @name 4. Callback Persistant State: Simple push/pop mecanism
 * 
 * Sometimes, one of the callbacks need to leave information for the next ones. If this is a simple integer (such as
 * an array size), you can use the functions described here. If not, you'll have to play with the complete cbps interface.
 * 
 * Here is an example:\verbatim
struct s_array {
  int length;
  int *data;
}
[...]
my_type=gras_datadesc_struct("s_array");
gras_datadesc_struct_append(my_type,"length", gras_datadesc_by_name("int"));
gras_datadesc_cb_field_send (my_type, "length", gras_datadesc_cb_push_int);

gras_datadesc_struct_append(my_type,"data",
                            gras_datadesc_array_dyn ("s_array::data",gras_datadesc_by_name("int"), gras_datadesc_cb_pop));
gras_datadesc_struct_close(my_type);
\endverbatim

 */
/*@{*/

void
gras_cbps_i_push(gras_cbps_t ps, int val);
int 
gras_cbps_i_pop(gras_cbps_t ps);

int gras_datadesc_cb_pop(gras_cbps_t vars, void *data);
void gras_datadesc_cb_push_int(gras_cbps_t vars, void *data);
void gras_datadesc_cb_push_uint(gras_cbps_t vars, void *data);
void gras_datadesc_cb_push_lint(gras_cbps_t vars, void *data);
void gras_datadesc_cb_push_ulint(gras_cbps_t vars, void *data);


/*@}*/

/** @name 5. Callback Persistant State: Full featured mecanism
 * 
 * Sometimes, one of the callbacks need to leave information for the next ones. If the simple push/pop mecanism
 * introduced in previous section isn't enough, you can always use this full featured one.
 */

/*@{*/

xbt_error_t
  gras_cbps_v_pop (gras_cbps_t            ps, 
		   const char            *name,
      	 /* OUT */ gras_datadesc_type_t  *ddt,
	 /* OUT */ void                 **res);
xbt_error_t
gras_cbps_v_push(gras_cbps_t            ps,
		 const char            *name,
		 void                  *data,
		 gras_datadesc_type_t   ddt);
void
gras_cbps_v_set (gras_cbps_t            ps,
		 const char            *name,
		 void                  *data,
		 gras_datadesc_type_t   ddt);

void *
gras_cbps_v_get (gras_cbps_t            ps, 
		 const char            *name,
       /* OUT */ gras_datadesc_type_t  *ddt);

void
gras_cbps_block_begin(gras_cbps_t ps);
void
gras_cbps_block_end(gras_cbps_t ps);

/* @} */
/*@}*/


/*******************************
 **** About data convertion ****
 *******************************/
int gras_arch_selfid(void); /* ID of this arch */


/*****************************
 **** NWS datadescription * FIXME: obsolete?
 *****************************/

/**
 * Basic types we can embeed in DataDescriptors.
 */
typedef enum
  {CHAR_TYPE, DOUBLE_TYPE, FLOAT_TYPE, INT_TYPE, LONG_TYPE, SHORT_TYPE,
   UNSIGNED_INT_TYPE, UNSIGNED_LONG_TYPE, UNSIGNED_SHORT_TYPE, STRUCT_TYPE}
  DataTypes;
#define SIMPLE_TYPE_COUNT 9

/*!  \brief Describe a collection of data.
 * 
** A description of a collection of #type# data.  #repetitions# is used only
** for arrays; it contains the number of elements.  #offset# is used only for
** struct members in host format; it contains the offset of the member from the
** beginning of the struct, taking into account internal padding added by the
** compiler for alignment purposes.  #members#, #length#, and #tailPadding# are
** used only for STRUCT_TYPE data; the #length#-long array #members# describes
** the members of the nested struct, and #tailPadding# indicates how many
** padding bytes the compiler adds to the end of the structure.
*/

typedef struct DataDescriptorStruct {
  DataTypes type;
  size_t repetitions;
  size_t offset;
  /*@null@*/ struct DataDescriptorStruct *members;
  size_t length;
  size_t tailPadding;
} DataDescriptor;
/** DataDescriptor for an array */
#define SIMPLE_DATA(type,repetitions) \
  {type, repetitions, 0, NULL, 0, 0}
/** DataDescriptor for an structure member */
#define SIMPLE_MEMBER(type,repetitions,offset) \
  {type, repetitions, offset, NULL, 0, 0}
/** DataDescriptor for padding bytes */
#define PAD_BYTES(structType,lastMember,memberType,repetitions) \
  sizeof(structType) - offsetof(structType, lastMember) - \
  sizeof(memberType) * repetitions

xbt_error_t
gras_datadesc_import_nws(const char           *name,
			 const DataDescriptor *desc,
			 unsigned long         howmany,
	       /* OUT */ gras_datadesc_type_t *dst);


END_DECL()

#endif /* GRAS_DATADESC_H */
