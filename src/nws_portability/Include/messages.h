/* $Id$ */

#ifndef MESSAGES_H
#define MESSAGES_H

/*
 * This package defines NWS functions for sending messages between hosts.
 */

#include <sys/types.h>  /* size_t */
#include "formatutil.h" /* DataDescriptor */
#include "protocol.h"   /* Socket */
#ifdef WITH_LDAP
#include <ldap.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This is the four-byte NWS version number.  The first two bytes are
 * major and minor release numbers; the meaning of the other two bytes is
 * undefined.
 */
#define NWS_VERSION 0x02030000


typedef unsigned int MessageType;

/** 
 * this is the message header: it contains the NWS version (if I don't
 * forget to update) so that we can try to be backward compatible, the
 * message type and the data size. Upwn recepit of a message this is
 * what is passed to the listener along with the open socket.
 */
typedef struct {
	unsigned int version;
	MessageType message;
	size_t dataSize;
} MessageHeader;


typedef void (*ListenFunction)(Socket *, MessageHeader);
typedef int (*LdapListenFunction)(Socket *);


/**
 * Waits up to #timeOut# seconds to see if a message comes in; if so,
 * calls the registered listener for that message (see
 * RegisterListener()).
 */
void
ListenForMessages(double timeOut);


/**
 * Indicates a desire that the function #listener# be called whenever a
 * #message# message comes in.  #image# is a printable message name; it's
 * used to log the arrival of the message.
 */
void
RegisterListener(MessageType message,
                 const char *image,
                 ListenFunction listener);


#ifdef WITH_LDAP
/*
** Indicates a desire that the function #listener# be called whenever an
** LDAP message comes in.  Only one LDAP listener may be active at a time.
*/
void
RegisterLdapListener(LdapListenFunction listener);


/*
** Sends an unsolicited notification of disconnection on *sd, then shut down
** the socket.  See the LDAP RFC for details on this message.
*/
void
SendLdapDisconnect (Socket *sd,
                    ber_int_t resultCode);
#endif


/**
 * Receives on #sd#, into #data#, the data described by the #howMany#-long
 * array of descriptors #description#.  Returns 1 if successful within
 * #timeOut# seconds, else 0.
 * If #timeOut# is negative adaptive timeouts will be used, if it's 0 no
 * timeouts will be used.
 */
int
RecvData(Socket sd,
         void *data,
         const DataDescriptor *description,
         size_t howMany,
         double timeOut);


/**
 * Waits for a #message# message to come in over #sd#.  If successful within
 * #timeOut# seconds, returns 1 and copies the size of the accompanying data
 * into #dataSize#; otherwise, returns 0.
 * If #timeOut# is negative adaptive timeouts will be used, if it's 0 no
 * timeouts will be used.
 */
int
RecvMessage(Socket sd,
            MessageType message,
            size_t *dataSize,
            double timeOut);


/**
 * Waits for a #message# message to come in over #sd#.  If successful within
 * #timeOut# seconds, returns 1 and copies the accompanying data into #data1#
 * and #data2#, which are described by the #howMany1# and #howMany3#-long
 * arrays of descriptors #description1# and #description2#, respectively;
 * otherwise, returns 0.
 * If #timeOut# is negative adaptive timeouts will be used, if it's 0 no
 * timeouts will be used.
 */
int
RecvMessageAndDatas(Socket sd,
                    MessageType message,
                    void *data1,
                    const DataDescriptor *description1,
                    size_t howMany1,
                    void *data2,
                    const DataDescriptor *description2,
                    size_t howMany2,
                    double timeOut);
#define RecvMessageAndData(sd,message,data,description,howMany,timeOut) \
  RecvMessageAndDatas(sd,message,data,description,howMany,NULL,NULL,0,timeOut)
 

/**
 * Sends #data#, described by the #howMany#-long array of descriptors
 * #description#.  Returns 1 if successful in #timeOut# seconds, else 0.  This
 * fuction allows callers to extend the data that accompanies messages beyond
 * the two items provided for by SendMessageAndDatas().  Note, however, that
 * since the data sent via a call to this function is not packaged with a
 * message, the receiver will not be able to determine the data size directly.
 * The caller must assure that the data size is known to the recipient, either
 * because its length is predetermined, or because its length was enclosed in a
 * previously-transmitted message.
 * If #timeOut# is negative adaptive timeouts will be used, if it's 0 no
 * timeouts will be used.
 */
int
SendData(Socket sd,
         const void *data,
         const DataDescriptor *description,
         size_t howMany,
         double timeOut);


/**
 * Sends a message of type #message# on #sd# followed by #data1# and #data2#,
 * which are described, respectively, by the #howMany1#-long and the
 * #howMany2#-long arrays of descriptors #description1# and #description2#.
 * Each data parameter may be NULL, in which case its description is ignored.
 * Returns 1 if successful within #timeOut# seconds, else 0.
 * If #timeOut# is negative adaptive timeouts will be used, if it's 0 no
 * timeouts will be used.
 */
int
SendMessageAndDatas(Socket sd,
                    MessageType message,
                    const void *data1,
                    const DataDescriptor *description1,
                    size_t howMany1,
                    const void *data2,
                    const DataDescriptor *description2,
                    size_t howMany2,
                    double timeOut);
#define SendMessage(sd,message,timeOut) \
   SendMessageAndDatas(sd,message,NULL,NULL,0,NULL,NULL,0,timeOut)
#define SendMessageAndData(sd,message,data,descriptor,howMany,timeOut) \
   SendMessageAndDatas(sd,message,data,descriptor,howMany,NULL,NULL,0,timeOut)

/**
 * reads the NWS header associated with in incoming message and returns
 * the message type #message# as an integer.  returns -1 if the read fails
 * If #timeOut# is negative adaptive timeouts will be used, if it's 0 no
 * timeouts will be used.
 */
int RecvMsgType(Socket sd, double timeout);

#ifdef __cplusplus
}
#endif

#endif /* MESSAGES_H */
