/* $Id$ */


#ifndef FORMATUTIL_H
#define FORMATUTIL_H


/*
** This package provides utilities for translating between host and network
** data formats.  It may be thought of as an extention of the Unix {hn}to{nh}*
** functions or as a lightweight version of XDR.  It handles big-to-little
** endian translations, integer size discrepencies, and conversions to/from
** IEEE floating point format.  It does *not* presently handle either non-two's
** complement integer formats or mixed-endian (e.g., little endian bytes, big
** endian words) ordering.  These are so rare and generally archaic that
** handling them seems not worth the added code.  Note: compiling the body of
** this package with NORMAL_FP_FORMAT defined will yield a tiny performance
** improvement and a small savings in code size by eliminating the IEEE
** conversion code.
*/


#include <stddef.h>    /* offsetof() */
#include <sys/types.h> /* size_t */


#ifdef __cplusplus
extern "C" {
#endif


/*
** Supported data types.  "Network format" characters are one byte and longs
** four bytes; otherwise, the formats are those used by Java: two byte shorts,
** four byte ints and floats, eight byte doubles; two's compliment integer and
** IEEE 754 floating point; most-significant bytes first.
*/
typedef enum
  {CHAR_TYPE, DOUBLE_TYPE, FLOAT_TYPE, INT_TYPE, LONG_TYPE, SHORT_TYPE,
   UNSIGNED_INT_TYPE, UNSIGNED_LONG_TYPE, UNSIGNED_SHORT_TYPE, STRUCT_TYPE}
  DataTypes;
#define SIMPLE_TYPE_COUNT 9
typedef enum {HOST_FORMAT, NETWORK_FORMAT} FormatTypes;


/*
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
  struct DataDescriptorStruct *members;
  size_t length;
  size_t tailPadding;
} DataDescriptor;
#ifndef NULL
#define NULL 0
#endif
#define SIMPLE_DATA(type,repetitions) {type, repetitions, 0, NULL, 0, 0}
#define SIMPLE_MEMBER(type,repetitions,offset) \
  {type, repetitions, offset, NULL, 0, 0}
#define PAD_BYTES(structType,lastMember,memberType,repetitions) \
  sizeof(structType) - offsetof(structType, lastMember) - \
  sizeof(memberType) * repetitions


/*
** Translates the data pointed to by #source# between host and network formats
** and stores the result in #destination#.  The contents of #source# are
** described by the #length#-long array #description#, and #sourceFormat#
** indicates whether we're translating from host format to network format or
** vice versa.  The caller must insure that the memory pointed to by
** #destination# is of sufficient size to contain the translated data.
*/
void
ConvertData(void *destination,
            const void *source,
            const DataDescriptor *description,
            size_t length,
            FormatTypes sourceFormat);


/*
** Returns the number of bytes required to hold the objects indicated by the
** data described by the #length#-long array #description#.  #hostFormat#
** indicates whether the host or network format size is desired.
*/
size_t
DataSize(const DataDescriptor *description,
         size_t length,
         FormatTypes format);


/*
** Returns 1 or 0 depending on whether or not the host format for #whatType#
** differs from the network format.
*/
int
DifferentFormat(DataTypes whatType);


/*
** Returns 1 or 0 depending on whether or not the host architecture stores
** data in little-endian (least significant byte first) order.
*/
int
DifferentOrder(void);


/*
** Returns 1 or 0 depending on whether or not the host data size for #whatType#
** differs from the network data size.
*/
int
DifferentSize(DataTypes whatType);


/*
** These two functions are a convenience that allows callers to work with
** homogenous blocks of data without setting up a DataDescriptor.  They perform
** the same function as the more general function on a block of #repetitions#
** occurrences of #whatType# data.
*/
void
HomogenousConvertData(void *destination,
                      const void *source,
                      DataTypes whatType,
                      size_t repetitions,
                      FormatTypes sourceFormat);
size_t
HomogenousDataSize(DataTypes whatType,
                   size_t repetitions,
                   FormatTypes format);


/*
** Copies #repetitions# data items of type #whatType# from #source# to
** #destination#, reversing the bytes of each item (i.e., translates between
** network and host byte order).  #format# indicates whether the data to be
** reversed is in network or host format.  This is an internal package function
** which is included in the interface for convenience.
*/
void
ReverseData(void *destination,
            const void *source,
            DataTypes whatType,
            int repetitions,
            FormatTypes format);


#ifdef __cplusplus
}
#endif


#endif
