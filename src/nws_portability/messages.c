/* $Id$ */


#include "config_portability.h"

#include <stddef.h>          /* offsetof() */
#include <stdlib.h>          /* free() malloc() REALLOC() */
#ifdef WITH_LDAP
#include <ldap.h>
#endif
#ifdef WITH_THREAD
#include <pthread.h>
#endif

#include "diagnostic.h"      /* FAIL() LOG() */
#include "protocol.h"        /* Socket functions */
#include "messages.h"
#include "osutil.h"
#include "timeouts.h"

#if defined(WITH_THREAD)
pthread_mutex_t Message_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Message_wait = PTHREAD_COND_INITIALIZER;
int Message_busy = 0;
pthread_t Message_holding = (pthread_t)-1;
#endif

static void *lock = NULL;			/* local mutex */

/*
 * Info on registered listeners.  #message# is the message for which #listener#
 * is registered; #image# the message image.  Note that, since we provide no
 * way to terminate listening for messages, we can simply expand the list by
 * one every time a new listener is registered.
 */
typedef struct {
	MessageType message;
	const char *image;
	ListenFunction listener;
} ListenerInfo;

static ListenerInfo *listeners = NULL;
static unsigned listenerCount = 0;
#ifdef WITH_LDAP
static LdapListenFunction ldapListener = NULL;
#endif

/*
 * A header sent with messages.  #version# is the NWS version and is presently
 * ignored, but it could be used for compatibility.  #message# is the actual
 * message.  #dataSize# is the number of bytes that accompany the message.
 */
static const DataDescriptor headerDescriptor[] =
	{SIMPLE_MEMBER(UNSIGNED_INT_TYPE, 1, offsetof(MessageHeader, version)),
	SIMPLE_MEMBER(UNSIGNED_INT_TYPE, 1, offsetof(MessageHeader, message)),
	SIMPLE_MEMBER(UNSIGNED_INT_TYPE, 1, offsetof(MessageHeader, dataSize))};
#define headerDescriptorLength 3


#if defined(WITH_THREAD)
 /*
  * ** uses mutex (which might spin) to test and set a global variable
  * **
  * ** note that the pthread condition variable allows waiting under a lock
  * ** and will block the calling thread
  * */
void LockMessageSystem()
{
	pthread_mutex_lock(&Message_lock);
	while(Message_busy == 1)
	{
		pthread_cond_wait(&Message_wait,&Message_lock);
	}

	Message_busy = 1;
	Message_holding = pthread_self(); /* debugging info only */

	pthread_mutex_unlock(&Message_lock);

	return;
}

void UnlockMessageSystem()
{
	pthread_mutex_lock(&Message_lock);

	Message_busy = 0;
	Message_holding = (pthread_t)-1; /* debugging info only */
	pthread_cond_signal(&Message_wait);

	pthread_mutex_unlock(&Message_lock);
	return;
}
#else
#define LockMessageSystem()
#define UnlockMessageSystem()
#endif

/*
 * Returns 1 or 0 depending on whether or not format conversion is required for
 * data with the format described by the #howMany#-long array #description#.
 *
 *  I believe is thread safe (DataSize & DifferentFOrmat are thread safe)
 */
static int
ConversionRequired(	const DataDescriptor *description,
			size_t howMany) {
	int i;

	if(DataSize(description, howMany, HOST_FORMAT) !=
			DataSize(description, howMany, NETWORK_FORMAT)) {
		return 1;
	}

	for(i = 0; i < howMany; i++) {
		if(description[i].type == STRUCT_TYPE) {
			if(ConversionRequired(description[i].members, description[i].length)) {
				return 1;
			}
		} else if(DifferentFormat(description[i].type))
			return 1;
	}

	return 0;
}


/* it should be thread safe (all the conversions routines should be
 * thread safe).
 * */
int
RecvData(	Socket sd,
		void *data,
		const DataDescriptor *description,
		size_t howMany,
		double timeOut) {

	void *converted;
	int convertIt;
	void *destination;
	int recvResult;
	size_t totalSize = DataSize(description, howMany, NETWORK_FORMAT);

	LockMessageSystem();

	converted = NULL;
	convertIt = ConversionRequired(description, howMany);

	if(convertIt) {
		converted = malloc(totalSize);
		if(converted == NULL) {
			UnlockMessageSystem();
			FAIL1("RecvData: memory allocation of %d bytes failed\n", totalSize);
		}
		destination = converted;
	} else {
		destination = data;
	}

	/* use adaptive timeouts? */
	if (timeOut < 0) {
		double start;

		/* adaptive timeout */
		start = CurrentTime();

		recvResult = RecvBytes(sd, destination, totalSize, GetTimeOut(RECV, Peer(sd), totalSize));
		/* we assume a failure is a timeout ... Shouldn't hurt
		 * too much getting a bigger timeout anyway */
		SetTimeOut(RECV, Peer(sd), CurrentTime()-start, totalSize, !recvResult);
	} else {
		recvResult = RecvBytes(sd, destination, totalSize, timeOut);
	}
	if (recvResult != 0) {
		if(DifferentOrder() || convertIt)
			ConvertData(data, destination, description, 
				howMany, NETWORK_FORMAT);

		if(converted != NULL)
			free(converted);
	}
	UnlockMessageSystem();

	return recvResult;
}


/* It should be thread safe (just read headerDescriptor[Lenght] and
 * RecvByte is thread safe) */
int
RecvMessage(Socket sd,
            MessageType message,
            size_t *dataSize,
            double timeOut) {
	char *garbage;
	MessageHeader header;

	if(!RecvData(	sd,
			(void *)&header,
			headerDescriptor,
			headerDescriptorLength,
			timeOut)) {
		FAIL("RecvMessage: no message received\n");
	}

	LockMessageSystem();

	if(header.message != message) {
		garbage = malloc(2048);
		if (garbage == NULL) {
			FAIL("RecvMessage: out of memory!");
		}
		/* get the rigth timeout */
		if (timeOut < 0) {
			timeOut = GetTimeOut(RECV, Peer(sd), 1);
		}
		while(header.dataSize > 0) {
			/* if we time out let's drop the socket */
			if (!RecvBytes(sd, garbage, (header.dataSize > sizeof(garbage)) ? sizeof(garbage) : header.dataSize, timeOut)) {
				DROP_SOCKET(&sd);
				WARN("RecvMessage: timeout on receiving non-handled message: dropping socket\n");
				break;
			}
			header.dataSize -= sizeof(garbage);
		}
		free(garbage);

		UnlockMessageSystem();
		FAIL1("RecvMessage: unexpected message %d received\n", header.message);
	}
	*dataSize = header.dataSize;
	UnlockMessageSystem();
	return(1);
}

/* it should be thread safe */
int
RecvMessageAndDatas(Socket sd,
                    MessageType message,
                    void *data1,
                    const DataDescriptor *description1,
                    size_t howMany1,
                    void *data2,
                    const DataDescriptor *description2,
                    size_t howMany2,
                    double timeOut) {
	size_t dataSize;

	if (RecvMessage(sd, message, &dataSize, timeOut) != 1) {
		/* failed to receive message: errors already printed out
		 * by RecvMessage() */
		return 0;
	}

	if(data1 != NULL) {
		if(!RecvData(sd, data1, description1, howMany1, timeOut)) {
			FAIL("RecvMessageAndDatas: data receive failed\n");
		}
	}

	if(data2 != NULL) {
		if(!RecvData(sd, data2, description2, howMany2, timeOut)) {
			FAIL("RecvMessageAndDatas: data receive failed\n");
		}
	}

	return(1);
}

/* 
 * waits for timeOut seconds for incoming messages and calls the
 * appropriate (registered) listener function.
 */
void
ListenForMessages(double timeOut) {

	MessageHeader header;
	int i, ldap;
	Socket sd;

	if(!IncomingRequest(timeOut, &sd, &ldap))
		return;

#ifdef WITH_LDAP
  /*
   ** WARNING!  Not sure if ldapListener is thread safe (yet)
   */
  if (ldap) {
    if (ldapListener == NULL) {
      WARN2("Unexpected LDAP message received from %s on %d\n",
            PeerName(sd), sd);
      SendLdapDisconnect(&sd, LDAP_UNAVAILABLE);
    }
    else {
      LOG2("Received LDAP message from %s on %d\n", PeerName(sd), sd);
      ldapListener(&sd);
    }
  } else {
#endif

	/* let's use the adaptive timeouts on receiving the header */
	if(!RecvData(sd, (void *)&header, headerDescriptor, headerDescriptorLength, -1)) {
		/* Likely a connection closed by the other side.  There
		 * doesn't seem to be any reliable way to detect this,
		 * and, for some reason, select() reports it as a
		 * connection ready for reading.  */
		DROP_SOCKET(&sd);
		return;
	}

	LockMessageSystem();

	for(i = 0; i < listenerCount; i++) {
		if(listeners[i].message == header.message) {
			LOG3("Received %s message from %s on %d\n", listeners[i].image, PeerName(sd), sd);
			listeners[i].listener(&sd, header);
			break;
		}
	}

	if(i == listenerCount) {
		WARN3("Unknown message %d received from %s on %d\n", header.message, PeerName(sd), sd);
		DROP_SOCKET(&sd);
	}
#ifdef WITH_LDAP
   }
#endif
}



/* regsiters the functions which should be called upon the receive of the
 * messageType message. Should be thread safe */
void
RegisterListener(MessageType message,
                 const char *image,
                 ListenFunction listener) {
	LockMessageSystem();
	if (!GetNWSLock(&lock)) {
		ERROR("RegisterListener: couldn't obtain the lock\n");
	}
	listeners = REALLOC(listeners, (listenerCount+1)*sizeof(ListenerInfo));
	listeners[listenerCount].message = message;
	listeners[listenerCount].image = image;
	listeners[listenerCount].listener = listener;
	listenerCount++;
	ReleaseNWSLock(&lock);
	UnlockMessageSystem();
}


#ifdef WITH_LDAP
void
RegisterLdapListener(LdapListenFunction listener) {
  ldapListener = listener;
}


void
SendLdapDisconnect (Socket *sd,
                    ber_int_t resultCode)
{
  /*
  ** Send an unsolicitied notice of disconnection, in compliance with the
  ** LDAP RFC.  This notice is pre-created to allow us to send it
  ** even if the lber libraries have failed (for example, due to a memory
  ** shortage).
  **
  ** abortMessage contains the unsolicited notice of disconnection.  
  ** abortMessageLength gives the length of the message, needed
  ** due to the message's embedded NULLs.  errorOffset gives the location
  ** of the error code within the message.
  **
  ** To create this message using the ber libraries, call ber_print as follows:
  **   ber_printf(ber, "{it{essts}}", 0, LDAP_RES_EXTENDED, resultCode,
  **     "", "", LDAP_TAG_EXOP_RES_OID, LDAP_NOTICE_OF_DISCONNECTION);
  ** The components of an unsolicited disconnect message (and hence the
  ** parameters for the above ber_printf) are as follows:
  **   messageID (must be zero for unsoliticed notification), protocolOp,
  **   resultCode, matchedDN, errorMessage, responseName (optional, and omitted
  **   for an unsolicited disconnect), response (with tag)
  **
  */
  static char abortMessage[] = "0$\x02\x01\x00x\x1f\x0a\x01\x02\x04\x00\x04"
    "\x00\x8a\x16" "1.3.6.1.4.1.1466.20036";
  int abortMessageLength = 38;
  int errorOffset = 9;
  abortMessage[errorOffset] = resultCode;
  SendBytes(*sd, abortMessage, abortMessageLength, -1);
  DROP_SOCKET(sd);
}
#endif

/* it should be thread safe (Convert*, SendBytes, DataSize abd
 * DifferentOrder are thread safe) */
int
SendData(Socket sd,
         const void *data,
         const DataDescriptor *description,
         size_t howMany,
         double timeOut) {

	void *converted;
	int sendResult;
	const void *source;
	size_t totalSize = DataSize(description, howMany, NETWORK_FORMAT);

	LockMessageSystem();
	converted = NULL;

	if(DifferentOrder() || ConversionRequired(description, howMany)) {
		converted = malloc(totalSize);
		if(converted == NULL) {
			UnlockMessageSystem();
			FAIL("SendData: memory allocation failed\n");
		}
		ConvertData(converted, data, description, howMany, HOST_FORMAT);
		source = converted;
	} else {
		source = data;
	}

	/* use adaptive timeouts? */
	if (timeOut < 0) {
		double start;

		/* adaptive timeout */
		start = CurrentTime();
		sendResult = SendBytes(sd, source, totalSize, GetTimeOut(SEND, Peer(sd),totalSize));
		/* we assume a failure is a timeout ... Shouldn't hurt
		 * too much getting a bigger timeout anyway */
		SetTimeOut(SEND, Peer(sd), CurrentTime()-start, totalSize, !sendResult);
	} else {
		sendResult = SendBytes(sd, source, totalSize, timeOut);
	}
	if(converted != NULL)
		free((void *)converted);

	UnlockMessageSystem();
	return sendResult;
}

/* it should be thread safe (SendData, DataSize are thread safe) */
int
SendMessageAndDatas(Socket sd,
                    MessageType message,
                    const void *data1,
                    const DataDescriptor *description1,
                    size_t howMany1,
                    const void *data2,
                    const DataDescriptor *description2,
                    size_t howMany2,
                    double timeOut) {

	MessageHeader header;

	LockMessageSystem();

	header.version = NWS_VERSION;
	header.message = message;
	header.dataSize = 0;
	if(data1 != NULL)
		header.dataSize += DataSize(description1, howMany1, NETWORK_FORMAT);
	if(data2 != NULL)
		header.dataSize += DataSize(description2, howMany2, NETWORK_FORMAT);

	UnlockMessageSystem();

	if(!SendData(sd,
			&header,
			headerDescriptor,
			headerDescriptorLength,
			timeOut)) {
		FAIL("SendMessageAndDatas: header send failed \n");
	}
	if((data1 != NULL) && !SendData(sd, data1, description1, howMany1, timeOut)) {
		FAIL("SendMessageAndDatas: data1 send failed\n");
	}
	if((data2 != NULL) && !SendData(sd, data2, description2, howMany2, timeOut)) {
		FAIL("SendMessageAndDatas: data2 send failed\n");
	}
	return 1;
}

/*
 * reads the NWS header associated with in incoming message and returns
 * the message type.  returns -1 if the read fails
 *
 * it should be thread safe (RecvData is thread safe and header* are only
 * read) 
 */
int RecvMsgType(Socket sd, double timeout)
{
	int status;
	MessageHeader header;

	status = RecvData(sd,
			  &header,
			  headerDescriptor,
			  headerDescriptorLength,
			  timeout);

	if(status <= 0) {
		return(-1);
	}

	return((int)header.message);
}
