/* $Id$ */


#include <string.h>  /* memcpy() */
#include "diagnostic.h"
#include "formatutil.h"
#include "osutil.h"


#ifndef NORMAL_FP_FORMAT
#define INCLUDE_FORMAT_CHECKING
#else
#ifndef NORMAL_INT_FORMAT
#define INCLUDE_FORMAT_CHECKING
#endif
#endif


static void *lock = NULL;			/* local mutex */
static const size_t HOST_SIZE[SIMPLE_TYPE_COUNT] =
	{sizeof(char), sizeof(double), sizeof(float), sizeof(int), sizeof(long),
	sizeof(short), sizeof(unsigned int), sizeof(unsigned long),
	sizeof(unsigned short)};
static const size_t NETWORK_SIZE[SIMPLE_TYPE_COUNT] =
	{1, 8, 4, 4, 4, 2, 4, 4, 2};

/* how this host treat int, floats and big/little endianness */
static int unusualFPFormat = 0;
static int unusualIntFormat = 0;
static int bytesReversed = -1;

#ifndef NORMAL_FP_FORMAT
/*
** Copies #source# to #destination#, converting between IEEE and host floating-
** point format.  #whatType# must be DOUBLETYPE or FLOATTYPE.  #hostToIEEE#
** indicates whether the conversion is from host format to IEEE or vice versa.
** The IEEE version of the data will be in big-endian byte order even if the
** host machine is little-endian.  For IEEE 754 floating point info, look at
** http://www.research.microsoft.com/~hollasch/cgindex/coding/ieeefloat.html
*/
/* It should be thread safe */
void
ConvertIEEE(	const void *destination,
		const void *source,
		DataTypes whatType,
		int hostToIEEE) {

#define DOUBLEBIAS 1023
#define QUADBIAS 16383
#define SINGLEBIAS 127

	struct DoublePrecision {
		unsigned sign : 1;
		unsigned exponent : 11;
		unsigned leading : 4;
		unsigned char mantissa[6];
	} doublePrecision;

	struct Expanded {
		unsigned char sign;
		int exponent;
		unsigned char mantissa[16];
	} expanded;

	struct QuadPrecision {
		unsigned sign : 1;
		unsigned exponent : 15;
		unsigned char mantissa[14];
	}; /* For future reference. */

	struct SinglePrecision {
		unsigned sign : 1;
		unsigned exponent : 8;
		unsigned leading : 7;
		unsigned char mantissa[2];
	} singlePrecision;

	double doubleValue;
	unsigned exponentBias;
	double factor;
	int i;
	size_t mantissaLength;

	if(whatType == DOUBLE_TYPE) {
		exponentBias = DOUBLEBIAS;
		mantissaLength = sizeof(doublePrecision.mantissa) + 1;
		factor = 16.0; /* 2.0 ^ bitsize(doublePrecision.leading) */
	} else {
		exponentBias = SINGLEBIAS;
		mantissaLength = sizeof(singlePrecision.mantissa) + 1;
		factor = 128.0; /* 2.0 ^ bitsize(singlePrecision.leading) */
	}

	if(hostToIEEE) {
		if(whatType == DOUBLE_TYPE)
			doubleValue = *(double *)source;
		else
			doubleValue = *(float *)source;

		if(doubleValue < 0.0) {
			expanded.sign = 1;
			doubleValue = -doubleValue;
		} else {
			expanded.sign = 0;
		}
		expanded.exponent = 0;
		if(doubleValue != 0.0) {
		/* Determine the exponent value by iterative shifts
		 * (mult/div by 2) */
			while(doubleValue >= 2.0) {
				expanded.exponent += 1;
				doubleValue /= 2.0;
			}
			while(doubleValue < 1.0) {
				expanded.exponent -= 1;
				doubleValue *= 2.0;
			}
			expanded.exponent += exponentBias;
			doubleValue -= 1.0;
		}

		/* Set the bytes of the mantissa by iterative shift and
		 * truncate. */
		for(i = 0; i < 16; i++) {
			doubleValue *= factor;
			expanded.mantissa[i] = (int)doubleValue;
			doubleValue -= expanded.mantissa[i];
			factor = 256.0;
		}
		/* Pack the expanded version into the destination. */
		if(whatType == DOUBLE_TYPE) {
			memcpy(doublePrecision.mantissa, &expanded.mantissa[1], sizeof(doublePrecision.mantissa));
			doublePrecision.leading = expanded.mantissa[0];
			doublePrecision.exponent = expanded.exponent;
			doublePrecision.sign = expanded.sign;
			*(struct DoublePrecision *)destination = doublePrecision;
		} else {
			memcpy(singlePrecision.mantissa, &expanded.mantissa[1], sizeof(singlePrecision.mantissa));
			singlePrecision.leading = expanded.mantissa[0];
			singlePrecision.exponent = expanded.exponent;
			singlePrecision.sign = expanded.sign;
			*(struct SinglePrecision *)destination = singlePrecision;
		}
	} else {
		/* Unpack the source into the expanded version. */
		if(whatType == DOUBLE_TYPE) {
			doublePrecision = *(struct DoublePrecision *)source;
			expanded.sign = doublePrecision.sign;
			expanded.exponent = doublePrecision.exponent;
			expanded.mantissa[0] = doublePrecision.leading;
			memcpy(&expanded.mantissa[1], doublePrecision.mantissa, sizeof(doublePrecision.mantissa));
		} else {
			singlePrecision = *(struct SinglePrecision *)source;
			expanded.sign = singlePrecision.sign;
			expanded.exponent = singlePrecision.exponent;
			expanded.mantissa[0] = singlePrecision.leading;
			memcpy(&expanded.mantissa[1], singlePrecision.mantissa, sizeof(singlePrecision.mantissa));
		}
		/* Set mantissa by via shifts and adds; allow for
		 * denormalized values. */
		doubleValue = (expanded.exponent == 0) ? 0.0 : 1.0;

		for(i = 0; i < mantissaLength; i++) {
			doubleValue += (double)expanded.mantissa[i] / factor;
			factor *= 256.0;
		}
		/* Set the exponent by iterative mults/divs by 2. */
		if(expanded.exponent == 0)
			; /* Nothing to do. */
		else if(expanded.exponent == (exponentBias * 2 + 1))
			/*
			 * An exponent of all ones represents one of
			 * three things: Infinity: mantissa of all zeros
			 * Indeterminate: sign of 1, mantissa leading one
			 * followed by all zeros NaN: all other values
			 * None of these can be reliably produced by C
			 * operations.  We might be able to get Infinity
			 * by dividing by zero, but, on a non-IEEE
			 * machine, we're more likely to cause some sort
			 * of floating-point exception.
      			 */
      			;
		else
			expanded.exponent -= exponentBias;

		if(expanded.exponent < 0) {
			for(i = expanded.exponent; i < 0; i++)
				doubleValue /= 2.0;
		} else {
			for(i = 0; i < expanded.exponent; i++)
			doubleValue *= 2.0;
		}

		if(expanded.sign)
			doubleValue *= -1.0;

		if(whatType == DOUBLE_TYPE)
			*(double *)destination = doubleValue;
		else
			*(float *)destination = doubleValue;
	}
}
#endif


/*
 * Copies the integer value of size #sourceSize# stored in #source# to the
 * #destinationSize#-long area #destination#.  #signedType# indicates whether
 * or not the source integer is signed; #lowOrderFirst# whether or not the
 * bytes run least-significant to most-significant.
 *
 * It should be thread safe (operates on local variables and calls mem*)
 */
void
ResizeInt(	void *destination,
		size_t destinationSize,
		const void *source,
		size_t sourceSize,
		int signedType,
		int lowOrderFirst) {

	unsigned char *destinationSign;
	int padding;
	int sizeChange = destinationSize - sourceSize;
	unsigned char *sourceSign;

	if(sizeChange == 0) {
		memcpy(destination, source, destinationSize);
	} else if(sizeChange < 0) {
		/* Truncate high-order bytes. */
		memcpy(destination, lowOrderFirst?source:((char*)source-sizeChange), destinationSize);
		if(signedType) {
			/* Make sure the high order bit of source and
			 * destination are the same */
			destinationSign = lowOrderFirst ? ((unsigned char*)destination + destinationSize - 1) : (unsigned char*)destination;
			sourceSign = lowOrderFirst ? ((unsigned char*)source + sourceSize - 1) : (unsigned char*)source;
			if((*sourceSign > 127) != (*destinationSign > 127)) {
				if(*sourceSign > 127)
					*destinationSign += 128;
				else
					*destinationSign -= 128;
			}
		}
	} else {
		/* Pad with zeros or extend sign, as appropriate. */
		if(!signedType)
			padding = 0;
		else {
			sourceSign = lowOrderFirst ? ((unsigned char*)source + sourceSize - 1) : (unsigned char*)source;
			padding = (*sourceSign > 127) ? 0xff : 0;
		}
		memset(destination, padding, destinationSize);
		memcpy(lowOrderFirst ? destination : ((char *)destination + sizeChange), source, sourceSize);
	}
}


/*
 * Copies #length# bytes from #from# to #to# in reverse order.  Will work
 * properly if #from# and #to# are the same address.
 *
 * It should be thread safe.
 */
void
ReverseBytes(	void *to,
		const void *from,
		size_t length) {

	char charBegin;
	const char *fromBegin;
	const char *fromEnd;
	char *toBegin;
	char *toEnd;

	for(fromBegin = (const char *)from, fromEnd = fromBegin + length - 1, toBegin = (char *)to, toEnd = toBegin + length - 1; fromBegin <= fromEnd; fromBegin++, fromEnd--, toBegin++, toEnd--) {
		charBegin = *fromBegin;
		*toBegin = *fromEnd;
		*toEnd = charBegin;
	}
}


void
ConvertData(	void *destination,
		const void *source,
		const DataDescriptor *description,
		size_t length,
		FormatTypes sourceFormat) {

	size_t destStructSize;
	int i;
	int j;
	size_t networkBytesConverted;
	char *nextDest;
	const char *nextSource;
	size_t sourceStructSize;

	networkBytesConverted = 0;

	for(i = 0; i < length; i++, description++) {
		if(sourceFormat == HOST_FORMAT) {
			nextDest = (char *)destination + networkBytesConverted;
			nextSource = (char *)source + description->offset;
		} else {
			nextDest = (char *)destination + description->offset;
			nextSource = (char *)source + networkBytesConverted;
		}
		if(description->type == STRUCT_TYPE) {
			if(sourceFormat == HOST_FORMAT) {
				destStructSize = DataSize(description->members, description->length, NETWORK_FORMAT);
				sourceStructSize = DataSize(description->members, description->length, HOST_FORMAT) + description->tailPadding;
			} else {
				destStructSize = DataSize(description->members, description->length, HOST_FORMAT) + description->tailPadding;
				sourceStructSize = DataSize(description->members, description->length, NETWORK_FORMAT);
			}
			for(j = 0; j < description->repetitions; j++) {
				ConvertData(nextDest, nextSource, description->members, description->length, sourceFormat);
				nextDest += destStructSize;
				nextSource += sourceStructSize;
			}
		} else {
			HomogenousConvertData(nextDest, nextSource, description->type, description->repetitions, sourceFormat);
		}
		networkBytesConverted += DataSize(description, 1, NETWORK_FORMAT);
	}
}


/* I believe is thread safe (HomogenousDataSize is thread safe) */
size_t
DataSize(	const DataDescriptor *description,
		size_t length,
		FormatTypes format) {

	int i;
	const DataDescriptor *lastMember;
	size_t totalSize;

	if(format == HOST_FORMAT) {
		lastMember = description;
		for(i = 0; i < length; i++) {
			if(description[i].offset > lastMember->offset) {
				lastMember = &description[i];
			}
		}
		return lastMember->offset + ((lastMember->type == STRUCT_TYPE) ?  ((DataSize(lastMember->members, lastMember->length, HOST_FORMAT) + lastMember->tailPadding) * lastMember->repetitions) : HomogenousDataSize(lastMember->type, lastMember->repetitions, HOST_FORMAT));
	} else {
		totalSize = 0;
		for(i = 0; i < length; i++, description++) {
			totalSize += (description->type == STRUCT_TYPE) ?  (DataSize(description->members, description->length, NETWORK_FORMAT) * description->repetitions) : HomogenousDataSize(description->type, description->repetitions, NETWORK_FORMAT);
		}
		return totalSize;
	}
}


/*
 * Internal call to be done only once. We find the fomrat of this host:
 * once done we re-use the results
 * 
 * Some of the checking could have been done at compile time, but to be
 * sure and to cover exoteric machine (doesn't ppc can behave as little
 * and bin endian?) we do it at runtime.
 *
 * I believe it is thread safe (uses the lock if it modifies global
 * variables)
 */
void
FirstCall() {

	/* we do the checking only once: next time bytesReversed will be
	 * set */
	if (bytesReversed == -1) {
		/* I think we could avoid this lock, but just to be safe */
		GetNWSLock(&lock);

		/* check endiannes */
		{
		typedef int IntTestType;
		union {
			IntTestType testInt;
			unsigned char bytes[sizeof(IntTestType)];
		} orderTester;

		orderTester.testInt = 1;
		bytesReversed = (orderTester.bytes[0] == 1);
		}

#ifdef INCLUDE_FORMAT_CHECKING
#	ifndef NORMAL_FP_FORMAT
		{
		typedef double FPTestType;
		union {
			FPTestType testFP;
			unsigned char bytes[sizeof(FPTestType)];
		} fpTester;
		
		memset(&fpTester, 0, sizeof(fpTester));
		/* Set sign, low-order bit of exponent and high-order bit 
		 * of mantissa. */
		fpTester.bytes[bytesReversed ? sizeof(FPTestType) - 1 : 0]=192;
		fpTester.bytes[bytesReversed ? sizeof(FPTestType) - 2 : 1] =
		(sizeof(FPTestType) == 4)  ? 128 :
		(sizeof(FPTestType) == 8)  ? 16 :
		(sizeof(FPTestType) == 16) ? 1 : 0;
		unusualFPFormat = fpTester.testFP != -4.0;
		}
#	endif

#	ifndef NORMAL_INT_FORMAT
		/* Converting non-twos-compliment is a pain, but
		 * detecting it is easy, so we go ahead and include a
		 * check and leave it to the caller to handle.  */
		{
		typedef int IntTestType;
		union {
			IntTestType testInt;
			unsigned char bytes[sizeof(IntTestType)];
		} intTester;

		intTester.testInt = -2;
		unusualIntFormat = ((unsigned int)intTester.bytes[0] +
			(unsigned int)intTester.bytes[sizeof(IntTestType) - 1]) != 509;
		}
#	endif
#endif

		ReleaseNWSLock(&lock);
	}
	return;
}

/* I believe is thread safe (FirstCall is thread safe and we just read
 * the global variables) */
int
DifferentFormat(DataTypes whatType) {

	FirstCall();
	return ((whatType == DOUBLE_TYPE) || (whatType == FLOAT_TYPE)) ?
			unusualFPFormat : unusualIntFormat;
}


/* I believe is thread safe (FirstCall is thread safe and we just read
 * the global variable) */
int
DifferentOrder(void) {

	FirstCall();
	return bytesReversed;
}


/* It should be thread sage (only reads global variables) */
int
DifferentSize(DataTypes whatType) {
	return HOST_SIZE[whatType] != NETWORK_SIZE[whatType];
}


/* It should be thread safe (ConvertIEEE, ReverseData
 * and mem* are thread safe) */
void
HomogenousConvertData(	void *destination,
			const void *source,
			DataTypes whatType,
			size_t repetitions,
			FormatTypes sourceFormat) {

	int bytesReverse = DifferentOrder();
	FormatTypes destFormat;
	const void *from;
	size_t fromSize;
	int i;
	void *to;
	size_t toSize;

	destFormat = (sourceFormat == HOST_FORMAT) ? NETWORK_FORMAT : HOST_FORMAT;
	fromSize = (sourceFormat == HOST_FORMAT) ?
		HOST_SIZE[whatType] : NETWORK_SIZE[whatType];
	toSize = (destFormat == HOST_FORMAT) ?
		HOST_SIZE[whatType] : NETWORK_SIZE[whatType];

#ifndef NORMAL_FP_FORMAT
	if(((whatType == DOUBLE_TYPE) || (whatType == FLOAT_TYPE)) &&
			(DifferentFormat(whatType) || (fromSize != toSize))) {
		for(i = 0, from = source, to = destination; i < repetitions; i++, from = (char *)from + fromSize, to = (char *)to + toSize) {
			ConvertIEEE(to, from, whatType, (sourceFormat == HOST_FORMAT));
		}
		/* Note: ConvertIEEE also handles byte ordering. */
		return;
	}
#endif

	if(fromSize != toSize) {
		for(i = 0, from = source, to = destination; i < repetitions; i++, from = (char *)from + fromSize, to = (char *)to + toSize) {
			ResizeInt(to, toSize, from, fromSize, whatType < UNSIGNED_INT_TYPE, (sourceFormat == HOST_FORMAT) && bytesReverse);
		}
		if(bytesReverse && (toSize > 1))
			ReverseData(destination, destination, whatType, repetitions, destFormat);
	} else if(bytesReverse && (toSize > 1))
		ReverseData(destination, source, whatType, repetitions, destFormat);
	else if(destination != source)
		memcpy(destination, source, fromSize * repetitions);
}


/* I believe is thread safe (*_SIZE are static and global but are
 * constant and initialize when created) */
size_t
HomogenousDataSize(	DataTypes whatType,
			size_t repetitions,
			FormatTypes format) {
	return ((format == HOST_FORMAT) ?
		HOST_SIZE[whatType] : NETWORK_SIZE[whatType]) * repetitions;
}


/* It should be thread safe (ReverseByte is thread safe */
void
ReverseData(	void *destination,
		const void *source,
		DataTypes whatType,
		int repetitions,
		FormatTypes format) {
	int i;
	size_t size;

	size = (format == HOST_FORMAT) ? HOST_SIZE[whatType] : NETWORK_SIZE[whatType];
	for(i = 0; i < repetitions; i++, destination = (char *)destination + size, source = (char *)source + size) {
		ReverseBytes(destination, source, size);
	}
}
