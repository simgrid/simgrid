/* $Id$ */


#ifndef DNSUTIL_H
#define DNSUTIL_H

#include "config_portability.h"

/**
 * This package defines some utilities for determining and converting DNS
 * machine names and IP addresses.
 */


#ifdef __cplusplus
extern "C" {
#endif


#define MAX_IP_IMAGE 15
/* Maximum text length of an IP address, i.e. strlen("255.255.255.255") */


/*
 * Common typedefs for all portability 
 */
typedef uint32_t IPAddress;
typedef int Socket;
#define NO_SOCKET ((Socket)-1)

/**
 * returns the address of the host connected to #sd#. Returns 0 on error.
 */
IPAddress
Peer(Socket sd);


/**
 * Returns the DNS name of the host connected to #sd#, or descriptive text if
 * #sd# is not an inter-host connection: returns NULL in case of error
 * The value returned needs to be freed.
 */
char *
PeerName_r(Socket sd);

/*
 * returns the port number on the other side of socket sd. -1 is returned
 * if pipes or unknown
 */
unsigned short
PeerNamePort(Socket sd);


/**
 * Converts #addr# into a printable string and returns the result.  You
 * are responsible to free the returned string: can return NULL (out of
 * memory condition).
 */
char *
IPAddressImage_r(IPAddress addr);


/**
 * Converts #addr# to a fully-qualified machine name and returns the result.
 * You are responsible to free the returned string: can return NULL 
 * (out of memory or error).
 */
char *
IPAddressMachine_r(IPAddress addr);

/*
 * Converts #machineOrAddress#, which may be either a DNS name or an IP address
 * image, into a list of addresses.  Copies the list into the #atMost#-long
 * array #addressList#.  Returns the number of addresses copied, or zero on
 * error.  #atMost# may be zero, in which case the function simply returns one
 * or zero depending on whether or not #machineOrAddress# is a valid machine
 * name or IP address image.
 */
int
IPAddressValues(const char *machineOrAddress,
                IPAddress *addressList,
                unsigned int atMost);
#define IPAddressValue(machineOrAddress,address) \
        IPAddressValues(machineOrAddress,address,1)
#define IsValidIP(machineOrAddress) IPAddressValues(machineOrAddress,NULL,0)


/**
 * Returns the fully-qualified name of the host machine, or NULL if the name
 * cannot be determined.  Always returns the same value, so multiple calls
 * cause no problems.
 */
const char *
MyMachineName(void);


/* DEPRECATED! These functions are going to go away: they are not thread
 * safe. */ 
const char *
IPAddressImage(IPAddress addr);
const char *
IPAddressMachine(IPAddress addr);
const char *
PeerName(Socket sd);


#ifdef __cplusplus
}
#endif


#endif
