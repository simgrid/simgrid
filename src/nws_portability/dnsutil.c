/* $Id$ */

#include "config_portability.h"

#include <stdio.h>
#include <stdlib.h>     /* REALLOC() */
#include <string.h>
#include <sys/types.h>  /* sometimes required for #include <sys/socket.h> */
#include <sys/socket.h> /* AF_INET */
#include <netinet/in.h> /* struct in_addr */
#include <arpa/inet.h>  /* inet_addr() inet_ntoa() */
#include <netdb.h>      /* {end,set}hostent() gethostby{addr,name}() */

#include "diagnostic.h"
#include "osutil.h"
#include "dnsutil.h"
#include "protocol.h"


/*
** NOTE: The man pages for {end,set}hostent() seem to imply that endhostent()
** needs/should only be called after sethostent(1).  Below, we call
** endhostent() after calling sethostent(0).  So far, this hasn't seemed to
** cause any problems, and it also appears to have squished a bug on some
** version of Unix where the O/S DNS cache was losing entries.
*/

static void *lock = NULL;		/* local lock */

/*
 * We cache host entries locally to avoid going to the DNS too often.  This
 * also gets around an old Solaris bug which leaks memory whenever dlopen is
 * called (such as on the dynamic DNS lookup library). 
 *
 * Cached values timeout after CACHE_TIMEOUT seconds.
 */
#define CACHE_TIMEOUT 1800
static unsigned int cacheCount = 0;
static struct hostent **cache = NULL;
static double *cacheTimeout = NULL;


/*
** Looks in the name and alias entries of #hostEntry# for a fully-qualified
** name.  Returns the fqn if found; otherwise, returns the name entry.
*/
static const char *
BestHostName(const struct hostent *hostEntry) {
	int i;

	if (hostEntry == NULL) {
		return NULL;
	}

	if (!strchr(hostEntry->h_name, '.')) {
		for (i = 0; hostEntry->h_aliases[i] != NULL; i++) {
			if (strchr(hostEntry->h_aliases[i], '.'))
				return hostEntry->h_aliases[i]; /* found! */
		}
	}

	/* If we don't have a fully-qualified name, do the best we can.  */
	return hostEntry->h_name;
}

/* 
 * free a struct hostent *. 
 */
void
FreeStructHostent(struct hostent *toFree) {
	int i;

	/* sanity check */
	if (toFree == NULL) {
		return;
	}

	/* let's start to free */
	if (toFree->h_aliases != NULL) {
		for (i=0; toFree->h_aliases[i] != NULL; i++) {
			free(toFree->h_aliases[i]);
		}
		free(toFree->h_aliases);
	}
	if (toFree->h_name != NULL) {
		free(toFree->h_name);
	}
	if (toFree->h_addr_list != NULL) {
		for (i=0; toFree->h_addr_list[i] != NULL; i++) {
			free(toFree->h_addr_list[i]);
		}
		free(toFree->h_addr_list);
	}
	free(toFree);

	return;
}

/*
 * copy a struct hostent * into a newly allocated struct hostent * that
 * you need to free when you are done using. Returns NULL in case of
 * errors.
 */
struct hostent *
CopyStructHostent(const struct hostent *orig) {
	struct hostent *ret;
	int i,j;

	/* sanity check */
	if (orig == NULL) {
		return NULL;
	}

	ret = (struct hostent *)MALLOC(sizeof(struct hostent));
	if (ret == NULL) {
		ERROR("Out of memory\n");
		return NULL;		/* out of memory */
	}
	memset((void *)ret, 0,  sizeof(struct hostent));
	
	/* make room for the name */
	ret->h_name = strdup(orig->h_name);
	if (ret->h_name == NULL) {
		free(ret);
		ERROR("Out of memory\n");
		return NULL; 		/* out of memory */
	}

	/* count aliases and copy them */
	for (i=0; orig->h_aliases != NULL && orig->h_aliases[i] != NULL; i++) {
		;
	}
	ret->h_aliases = (char **)MALLOC(sizeof(char *) * (i+1));
	if (ret->h_aliases == NULL) {
		FreeStructHostent(ret);
		ERROR("Out of memory\n");
		return NULL;
	}
	for (j=0; j < i; j++) {
		ret->h_aliases[j] = strdup(orig->h_aliases[j]);
		if (ret->h_aliases[j] == NULL) {
			FreeStructHostent(ret);
			ERROR("Out of memory\n");
			return NULL;
		}
	}
	ret->h_aliases[i] = NULL;

	/* copy the easy stuff */
	ret->h_addrtype = orig->h_addrtype;
	ret->h_length = orig->h_length;

	/* copy the addresses */
	for (i=0; orig->h_addr_list != NULL && orig->h_addr_list[i] != NULL; i++) {
		;
	}
	ret->h_addr_list = (char **)MALLOC(sizeof(struct in_addr *) * (i+1));
	if (ret->h_addr_list == NULL) {
		FreeStructHostent(ret);
		ERROR("Out of memory\n");
		return NULL;
	}
	for (j=0; j < i; j++) {
		ret->h_addr_list[j] = (char *)MALLOC(ret->h_length + 1);
		if (ret->h_addr_list[j] == NULL) {
			FreeStructHostent(ret);
			ERROR("Out of memory\n");
			return NULL;
		}
		memcpy(ret->h_addr_list[j], orig->h_addr_list[j], ret->h_length);
	}
	ret->h_addr_list[i] = NULL;

	/* done */
	return ret;
}


/*
 * Appends #hostEntry# (not a copy) to the global map cache. 
 *
 * cache is a global structure and need to be protected by locks to be
 * thread safe.
 */
void
CacheHostent(struct hostent *hostEntry) {
	int ind;
	struct hostent **extendedCache;
	double *tmp_touts, now;


	/* sanity check */
	if (hostEntry == NULL) {
		return;
	}
	
	/* look for an expired entry */
	now = CurrentTime();
	if (!GetNWSLock(&lock)) {
		ERROR("CacheHostent: couldn't obtain the lock\n");
	}
	for (ind=0; ind < cacheCount; ind++) {
		if (now > cacheTimeout[ind]) {
			FreeStructHostent(cache[ind]);
			break;
		}
	}

	if (ind >= cacheCount) {
		/* no found, we need to add memory */
		extendedCache = (struct hostent**)REALLOC(cache, sizeof(struct hostent *) * (cacheCount + 1));
		if(extendedCache == NULL) {
			ReleaseNWSLock(&lock);
			ERROR("Out of memory\n");
			return;
		}
		cache = extendedCache;
		tmp_touts = (double *)REALLOC(cacheTimeout, sizeof(double) * (cacheCount +1));
		if(tmp_touts == NULL) {
			ReleaseNWSLock(&lock);
			ERROR("Out of memory\n");
			return; 
		}
		cacheTimeout = tmp_touts;
		ind = cacheCount++;
	}

	cache[ind] = CopyStructHostent(hostEntry);
	if (cache[ind] == NULL) {
		cacheTimeout[ind] = 1;
	} else {
		cacheTimeout[ind] = now + CACHE_TIMEOUT;
	}
	ReleaseNWSLock(&lock);

	return;
}


/*
 * Searches the DNS mapping cache for #address#, adding a new entry 
 * if needed.  Returns a copy of the the mapping entry, or 
 * NULL on error. The memory returned needs to be freed.
 */
struct hostent*
LookupByAddress(IPAddress address) {
	struct in_addr addr;
	struct hostent *tmp, *addrEntry;
	struct in_addr **cachedAddr;
	int i;
	double now;

	/* look if we have it in the cache and it's not expired */
	now = CurrentTime();
	if (!GetNWSLock(&lock)) {
		ERROR("LookupByAddress: couldn't obtain the lock\n");
	}
	for(i = 0; i < cacheCount; i++) {
		if (now > cacheTimeout[i]) {
			continue;
		}
		for(cachedAddr = (struct in_addr**)cache[i]->h_addr_list;
				*cachedAddr != NULL;
				cachedAddr++) {
			if((**cachedAddr).s_addr == address) {
				tmp = CopyStructHostent(cache[i]);
				ReleaseNWSLock(&lock);
				return tmp;
			}
		}
	}

	addr.s_addr = address;
	sethostent(0);
	tmp = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
	addrEntry = CopyStructHostent(tmp);
	endhostent();
	
	ReleaseNWSLock(&lock);

	if(addrEntry != NULL && addrEntry->h_length != sizeof(struct in_addr)) {
		/* We don't (yet) handle non-in_addr addresses. */
		FreeStructHostent(addrEntry);
		addrEntry = NULL;
	}

	/* the NULL case is handled by the functions directly */
	CacheHostent(addrEntry);

	/* addrEntry will need to be freed after */
	return addrEntry;
}


/*
 * Searches the DNS mapping cache for #name#, adding a new entry if needed.
 * Returns a pointer to the mapping entry, or NULL on error. The returned
 * value need to be freed.
 */
struct hostent*
LookupByName(const char *name) {

	char **cachedName;
	char **extendedAliases;
	struct hostent *tmp, *nameEntry;
	double now;
	int i, listLen;

	/* look in the cache for non expired entry */
	now = CurrentTime();
	if (!GetNWSLock(&lock)) {
		ERROR("LookupByName: failed to obtain the lock\n");
	}
	for(i = 0; i < cacheCount; i++) {
		if (now > cacheTimeout[i]) {
			continue;
		}
 		if(strcasecmp(name, cache[i]->h_name) == 0) {
			tmp = CopyStructHostent(cache[i]);
			ReleaseNWSLock(&lock);
 			return tmp;
		}
		for(cachedName = cache[i]->h_aliases; *cachedName != NULL; cachedName++) {
			if(strcasecmp(*cachedName, name) == 0) {
				tmp = CopyStructHostent(cache[i]);
				ReleaseNWSLock(&lock);
				return tmp;
			}
		}
	}

	sethostent(0);
	tmp = gethostbyname(name);
	nameEntry = CopyStructHostent(tmp);
	endhostent();

	ReleaseNWSLock(&lock);

	if(nameEntry == NULL) {
		return NULL;
	} else if(nameEntry->h_length != sizeof(struct in_addr)) {
		FreeStructHostent(nameEntry);
		return NULL; /* We don't (yet) handle non-in_addr addresses. */
	}

	/* We extend cached entries' h_aliases lists to include nicknames. */
	for(listLen = 0; nameEntry->h_aliases[listLen] != NULL; listLen++) {
		;
	}
	extendedAliases = (char **)REALLOC(nameEntry->h_aliases, sizeof(char **) * (listLen + 2));
	if(extendedAliases != NULL) {
		extendedAliases[listLen] = strdup(name);
		if (extendedAliases[listLen] != NULL) {
			extendedAliases[listLen + 1] = NULL;
			nameEntry->h_aliases = extendedAliases;
		}
	}

	/* let's invalidate the old (if there is) entry in the cache */
	if (!GetNWSLock(&lock)) {
		ERROR("LookupByName: failed to obtain the lock\n");
	}
	for(i = 0; i < cacheCount; i++) {
		if (now > cacheTimeout[i]) {
			continue;
		}
		if(strcmp(nameEntry->h_name, cache[i]->h_name) == 0) {
			cacheTimeout[i] = 1;
			break;
		}
	}
	ReleaseNWSLock(&lock);

	/* update the cache entry */
	CacheHostent(nameEntry);

	/* nameEntry will need to be freed */
	return nameEntry;
}

/* thread safe */
IPAddress
Peer(Socket sd) {
	struct sockaddr peer;
	SOCKLEN_T peer_size = sizeof(peer);
	int returnValue = 0;

	if (!IsPipe(sd) && (getpeername(sd, &peer, &peer_size) == 0)) {
		if (peer.sa_family == AF_INET) {
			returnValue = (((struct sockaddr_in *)&peer)->sin_addr.s_addr);
		}
	}

	return returnValue;
}

/* thread safe */
char *
PeerName_r(Socket sd) {
	struct sockaddr peer;
	SOCKLEN_T peer_size = sizeof(peer);
	char *returnValue;

	if (IsPipe(sd)) {
		returnValue = strdup("pipe");
	} else if (getpeername(sd, &peer, &peer_size) < 0) {
		returnValue = strdup("unknown");
	} else {
		if (peer.sa_family != AF_INET) {
			returnValue = strdup("unknown");
		} else {
			returnValue = IPAddressImage_r(((struct sockaddr_in *)&peer)->sin_addr.s_addr);
		}
	}

	if (returnValue == NULL) {
		ERROR("PeerName_r: out of memory\n");
	}
	return returnValue;
}


/* thread safe */
unsigned short
PeerNamePort(Socket sd) {
	unsigned short tmp;
	struct sockaddr peer;
	SOCKLEN_T peer_size = sizeof(peer);

	/* connectedPipes is global */
	if (!GetNWSLock(&lock)) {
		ERROR("PeerNamePort: failed to obtain the lock\n");
	}
	tmp = (IsPipe(sd) ? -1 : (getpeername(sd, &peer, &peer_size) < 0) ?  -1
			: ((struct sockaddr_in *)&peer)->sin_port);
	ReleaseNWSLock(&lock);
	return tmp;
}


/* thread safe: we allocate memory for the returned char * */
char *
IPAddressImage_r(IPAddress addr) {
	struct in_addr addrAsInAddr;
	char *returned, *tmp;

	addrAsInAddr.s_addr = addr;

	if (!GetNWSLock(&lock)) {
		ERROR("IPAddressImage_r: failed to obtain the lock\n");
	}
	tmp = inet_ntoa(addrAsInAddr);
	if (tmp != NULL) {
		returned = strdup(inet_ntoa(addrAsInAddr));
	} else {
		returned = strdup("unknown");
	}
	ReleaseNWSLock(&lock);

	if (returned == NULL) {
		ERROR("IPAddressImage_r: out of memory\n");
	}

	return returned;
}


/* thread safe: we allocate memory for the returned char. NULL have to be
 * checked by the caller. */
char *
IPAddressMachine_r(IPAddress addr) {
	struct hostent *hostEntry;
	char *returnValue;

	hostEntry = LookupByAddress(addr);
	if (hostEntry == NULL) {
		return NULL;
	}

	returnValue = strdup(BestHostName(hostEntry));

	/* free allocated memory */
	FreeStructHostent(hostEntry);

	return returnValue;
}


/* thread safe. */
int
IPAddressValues(const char *machineOrAddress,
                IPAddress *addressList,
                unsigned int atMost) {
	struct hostent *hostEntry;
	int i = 0, itsAnAddress = 0;
	IPAddress temp = 0;
#ifdef HAVE_INET_ATON
	struct in_addr in;

	if (inet_aton(machineOrAddress, &in) != 0) {
		itsAnAddress = 1;
		temp = in.s_addr;
	}
#else
	/* inet_addr() has the weird behavior of returning an unsigned
	 * quantity but using -1 as an error value.  Furthermore, the
	 * value returned is sometimes int and sometimes long,
	 * complicating the test.  Once inet_aton() is more widely
	 * available, we should switch to using it instead.  */
	temp = inet_addr(machineOrAddress);
	if (temp != -1) {
		itsAnAddress = 1;
	}
#endif
	if(itsAnAddress && (atMost == 1)) {
		*addressList = temp;
		return 1;
	}

	hostEntry = itsAnAddress ?  LookupByAddress(temp) : LookupByName(machineOrAddress);

	/* sanity check */
	if(hostEntry == NULL) {
		return 0;
	} 

	/* if atMost == 0 means we are checking if the address is
	 * correct. It is */
	if(atMost == 0) {
		i = 1;
	} 

	for(; i < atMost && hostEntry->h_addr_list[i] != NULL; i++) {
		memcpy(&addressList[i], hostEntry->h_addr_list[i], hostEntry->h_length);
	}

	FreeStructHostent(hostEntry);
	return i;
}


/* well, the name is always the same so we can use a static variable.
 * Changed to never return NULL but at the very worse localhost. */
const char *
MyMachineName(void) {

	struct hostent *myEntry;
	static char returnValue[255] = "";

	/* If we have a value in returnValue, done */
	if(returnValue[0] != '\0') {
		return returnValue;
	}

	/* try the simple case first */
	if(gethostname(returnValue, sizeof(returnValue)) == -1) {
		ERROR("gethostname() failed! using localhost instead.\n");
		/* setting the name to a safe bet */
		if (!GetNWSLock(&lock)) {
			ERROR("MyMachineName: failed to obtain the lock\n");
		}
		strncpy(returnValue, "localhost", sizeof(returnValue));
		ReleaseNWSLock(&lock);
	} else if(!strchr(returnValue, '.')) {
		/* Okay, that didn't work; try the DNS. */
		myEntry = LookupByName(returnValue);
		if(myEntry != NULL) {
			if (!GetNWSLock(&lock)) {
				ERROR("MyMachineName: failed to obtain the lock\n");
			}
			strncpy(returnValue,BestHostName(myEntry),sizeof(returnValue));
			returnValue[sizeof(returnValue) - 1] = '\0';
			ReleaseNWSLock(&lock);
			FreeStructHostent(myEntry);
		}
	}

	return returnValue;
}


/* DEPRECATED: these are not thread-safe */
const char *
IPAddressImage(IPAddress addr) {
	struct in_addr addrAsInAddr;
	addrAsInAddr.s_addr = addr;
	return inet_ntoa(addrAsInAddr);
}
const char *
IPAddressMachine(IPAddress addr) {
	struct hostent *hostEntry;
	static char returnValue[63 + 1];
	hostEntry = LookupByAddress(addr);
	strncpy(returnValue,
		(hostEntry == NULL) ? "" : BestHostName(hostEntry),
		sizeof(returnValue));
	FreeStructHostent(hostEntry);
	return returnValue;
}

const char *
PeerName(Socket sd) {
   struct sockaddr peer;
   SOCKLEN_T peer_size = sizeof(peer);
   static char returnValue[200];
   strcpy(returnValue, IsPipe(sd) ? "pipe" :
		(getpeername(sd, &peer, &peer_size) < 0) ?
		"(unknown)" :
		inet_ntoa(((struct sockaddr_in *)&peer)->sin_addr));
   return returnValue;
}


