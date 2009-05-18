/* $Id$ */

/* gras message types and callback registering and retrieving               */

/* Copyright (c) 2003, 2004, 2005, 2006, 2007 Martin Quinson.               */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "gras/Msg/msg_private.h"
#include "gras/Virtu/virtu_interface.h"
#include "gras/DataDesc/datadesc_interface.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_msg,gras,"High level messaging");

xbt_set_t _gras_msgtype_set = NULL;


/* ******************************************************************** */
/*                      MESSAGE TYPES                                   */
/* ******************************************************************** */


/* Reclamed memory */
void gras_msgtype_free(void *t) {
	gras_msgtype_t msgtype=(gras_msgtype_t)t;
	if (msgtype) {
		free(msgtype->name);
		free(msgtype);
	}
}
/**
 * Dump all declared message types (debugging purpose)
 */
void gras_msgtype_dumpall(void) {
	xbt_set_cursor_t cursor;
	gras_msgtype_t msgtype=NULL;

	INFO0("Dump of all registered messages:");
	xbt_set_foreach(_gras_msgtype_set, cursor, msgtype) {
		INFO6("  Message name: %s (v%d) %s; %s%s%s",
				msgtype->name, msgtype->version, e_gras_msg_kind_names[msgtype->kind],
				gras_datadesc_get_name(msgtype->ctn_type),
				(msgtype->kind==e_gras_msg_kind_rpccall ? " -> ":""),
				(msgtype->kind==e_gras_msg_kind_rpccall ? gras_datadesc_get_name(msgtype->answer_type) : ""));
	}
}


/**
 * make_namev:
 *
 * Returns the versionned name of the message.
 *   It's a newly allocated string, make sure to free it.
 */
static char *make_namev(const char *name, short int ver) {
	if (!ver)
		return xbt_strdup(name);

	return bprintf("%s_v%d",name,ver);
}

/* Internal function doing the crude work of registering messages */
void
gras_msgtype_declare_ext(const char           *name,
		short int             version,
		e_gras_msg_kind_t     kind,
		gras_datadesc_type_t  payload_request,
		gras_datadesc_type_t  payload_answer) {

	gras_msgtype_t msgtype=NULL;
	char *namev=make_namev(name,version);
	volatile int found = 0;
	xbt_ex_t e;

	TRY {
		msgtype = (gras_msgtype_t)xbt_set_get_by_name(_gras_msgtype_set,namev);
		found = 1;
	} CATCH(e) {
		if (e.category != not_found_error) {
			xbt_free(namev);
			RETHROW;
		}
		xbt_ex_free(e);
	}

	if (found) {
		VERB2("Re-register version %d of message '%s' (same kind & payload, ignored).",
				version, name);
		xbt_assert3(msgtype->kind == kind,
				"Message %s re-registered as a %s (it was known as a %s)",
				namev,e_gras_msg_kind_names[kind],e_gras_msg_kind_names[msgtype->kind]);
		xbt_assert3(!gras_datadesc_type_cmp(msgtype->ctn_type, payload_request),
				"Message %s re-registred with another payload (%s was %s)",
				namev,gras_datadesc_get_name(payload_request),
				gras_datadesc_get_name(msgtype->ctn_type));

		xbt_assert3(!gras_datadesc_type_cmp(msgtype->answer_type, payload_answer),
				"Message %s re-registred with another answer payload (%s was %s)",
				namev,gras_datadesc_get_name(payload_answer),
				gras_datadesc_get_name(msgtype->answer_type));

		xbt_free(namev);
		return ; /* do really ignore it */
	}

	VERB4("Register version %d of message '%s' "
			"(payload: %s; answer payload: %s).",
			version, name, gras_datadesc_get_name(payload_request),
			gras_datadesc_get_name(payload_answer));

	msgtype = xbt_new(s_gras_msgtype_t,1);
	msgtype->name = namev;
	msgtype->name_len = strlen(namev);
	msgtype->version = version;
	msgtype->kind = kind;
	msgtype->ctn_type = payload_request;
	msgtype->answer_type = payload_answer;

	xbt_set_add(_gras_msgtype_set, (xbt_set_elm_t)msgtype,
			&gras_msgtype_free);
}


/** @brief declare a new message type of the given name. It only accepts the given datadesc as payload
 *
 * @param name: name as it should be used for logging messages (must be uniq)
 * @param payload: datadescription of the payload
 */
void gras_msgtype_declare(const char           *name,
		gras_datadesc_type_t  payload) {
	gras_msgtype_declare_ext(name, 0, e_gras_msg_kind_oneway, payload, NULL);
}



/** @brief declare a new versionned message type of the given name and payload
 *
 * @param name: name as it should be used for logging messages (must be uniq)
 * @param version: something like versionning symbol
 * @param payload: datadescription of the payload
 *
 * Registers a message to the GRAS mechanism. Use this version instead of
 * gras_msgtype_declare when you change the semantic or syntax of a message and
 * want your programs to be able to deal with both versions. Internally, each
 * will be handled as an independent message type, so you can register
 * differents for each of them.
 */
void
gras_msgtype_declare_v(const char           *name,
		short int             version,
		gras_datadesc_type_t  payload) {

	gras_msgtype_declare_ext(name, version,
			e_gras_msg_kind_oneway, payload, NULL);
}

/** @brief retrieve an existing message type from its name (raises an exception if it does not exist). */
gras_msgtype_t gras_msgtype_by_name (const char *name) {
	return gras_msgtype_by_namev(name,0);
}
/** @brief retrieve an existing message type from its name (or NULL if it does not exist). */
gras_msgtype_t gras_msgtype_by_name_or_null (const char *name) {
	xbt_ex_t e;
	gras_msgtype_t res = NULL;

	TRY {
		res = gras_msgtype_by_namev(name,0);
	} CATCH(e) {
		res = NULL;
		xbt_ex_free(e);
	}
	return res;
}

/** @brief retrieve an existing message type from its name and version. */
gras_msgtype_t gras_msgtype_by_namev(const char      *name,
		short int        version) {
	gras_msgtype_t res = NULL;
	char *namev = make_namev(name,version);
	volatile int found=0;
	xbt_ex_t e;

	TRY {
		res = (gras_msgtype_t)xbt_set_get_by_name(_gras_msgtype_set, namev);
		found=1;
	} CATCH(e) {
		xbt_ex_free(e);
	}
	if (!found)
		THROW1(not_found_error,0,"No registred message of that name: %s",name);

	free(namev);

	return res;
}
/** @brief retrieve an existing message type from its name and version. */
gras_msgtype_t gras_msgtype_by_id(int id) {
	return (gras_msgtype_t)xbt_set_get_by_id(_gras_msgtype_set, id);
}



/* ******************************************************************** */
/*                        CALLBACKS                                     */
/* ******************************************************************** */
void
gras_cbl_free(void *data){
	gras_cblist_t *list=*(void**)data;
	if (list) {
		xbt_dynar_free(&( list->cbs ));
		free(list);
	}
}

/** \brief Bind the given callback to the given message type
 *
 * Several callbacks can be attached to a given message type.
 * The lastly added one will get the message first, and
 * if it returns a non-null value, the message will be passed to the
 * second one. And so on until one of the callbacks accepts the message.
 */
void
gras_cb_register_(gras_msgtype_t msgtype,
		gras_msg_cb_t cb) {
	gras_msg_procdata_t pd=
		(gras_msg_procdata_t)gras_libdata_by_id(gras_msg_libdata_id);
	gras_cblist_t *list=NULL;
	unsigned int cpt;

	DEBUG2("Register %p as callback to '%s'",cb,msgtype->name);

	/* search the list of cb for this message on this host (creating if NULL) */
	xbt_dynar_foreach(pd->cbl_list,cpt,list) {
		if (list->id == msgtype->code) {
			break;
		} else {
			list=NULL;
		}
	}
	if (!list) {
		/* First cb? Create room */
		list = xbt_new(gras_cblist_t,1);
		list->id = msgtype->code;
		list->cbs = xbt_dynar_new(sizeof(gras_msg_cb_t), NULL);
		xbt_dynar_push(pd->cbl_list,&list);
	}

	/* Insert the new one into the set */
	xbt_dynar_insert_at(list->cbs,0,&cb);
}

/** \brief Unbind the given callback from the given message type */
void
gras_cb_unregister_(gras_msgtype_t msgtype,
		gras_msg_cb_t cb) {

	gras_msg_procdata_t pd=
		(gras_msg_procdata_t)gras_libdata_by_id(gras_msg_libdata_id);
	gras_cblist_t *list;
	gras_msg_cb_t cb_cpt;
	unsigned int cpt;
	int found = 0;

	/* search the list of cb for this message on this host */
	xbt_dynar_foreach(pd->cbl_list,cpt,list) {
		if (list->id == msgtype->code) {
			break;
		} else {
			list=NULL;
		}
	}

	/* Remove it from the set */
	if (list) {
		xbt_dynar_foreach(list->cbs,cpt,cb_cpt) {
			if (cb == cb_cpt) {
				xbt_dynar_cursor_rm(list->cbs, &cpt);
				found = 1;
			}
		}
	}
	if (!found)
		VERB1("Ignoring removal of unexisting callback to msg id %d",
				msgtype->code);
}

/** \brief Retrieve the expeditor of the message */
gras_socket_t gras_msg_cb_ctx_from(gras_msg_cb_ctx_t ctx) {
	return ctx->expeditor;
}
/* \brief Creates a new message exchange context (user should never have to) */
gras_msg_cb_ctx_t gras_msg_cb_ctx_new(gras_socket_t expe,
		gras_msgtype_t msgtype,
		unsigned long int ID,
		int answer_due,
		double timeout) {
	gras_msg_cb_ctx_t res=xbt_new(s_gras_msg_cb_ctx_t,1);
	res->expeditor = expe;
	res->msgtype = msgtype;
	res->ID = ID;
	res->timeout = timeout;
	res->answer_due = answer_due;

	return res;
}
/* \brief Frees a message exchange context
 *
 * This function is mainly useful with \ref gras_msg_wait_or, ie seldom.
 */
void gras_msg_cb_ctx_free(gras_msg_cb_ctx_t ctx) {
	free(ctx);
}
