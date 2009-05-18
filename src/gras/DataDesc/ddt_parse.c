/* $Id$ */

/* DataDesc/ddt_parse.c -- automatic parsing of data structures             */

/* Copyright (c) 2003 Arnaud Legrand.                                       */
/* Copyright (c) 2003, 2004 Martin Quinson.                                 */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <ctype.h> /* isdigit */

#include "xbt/ex.h"
#include "gras/DataDesc/datadesc_private.h"
#include "gras/DataDesc/ddt_parse.yy.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_ddt_parse,gras_ddt,
		"Parsing C data structures to build GRAS data description");

typedef struct s_type_modifier{
	short is_long;
	int is_unsigned:1;
	int is_short:1;

	int is_struct:1;
	int is_union:1;
	int is_enum:1;

	int is_ref:1;

	int is_dynar:2;
	int is_matrix:2;
} s_type_modifier_t,*type_modifier_t;

typedef struct s_field {
	gras_datadesc_type_t type;
	char *type_name;
	char *name;
	s_type_modifier_t tm;
} s_identifier_t;

extern char *gras_ddt_parse_text; /* text being considered in the parser */

/* local functions */
static void parse_type_modifier(type_modifier_t type_modifier)  {
	XBT_IN;
	do {
		if (gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_STAR) {
			/* This only used when parsing 'short *' since this function returns when int, float, double,... is encountered */
			DEBUG0("This is a reference");
			type_modifier->is_ref++;

		} else if (!strcmp(gras_ddt_parse_text,"unsigned")) {
			DEBUG0("This is an unsigned");
			type_modifier->is_unsigned = 1;

		} else if (!strcmp(gras_ddt_parse_text,"short")) {
			DEBUG0("This is short");
			type_modifier->is_short = 1;

		} else if (!strcmp(gras_ddt_parse_text,"long")) {
			DEBUG0("This is long");
			type_modifier->is_long++; /* handle "long long" */

		} else if (!strcmp(gras_ddt_parse_text,"struct")) {
			DEBUG0("This is a struct");
			type_modifier->is_struct = 1;

		} else if (!strcmp(gras_ddt_parse_text,"union")) {
			DEBUG0("This is an union");
			type_modifier->is_union = 1;

		} else if (!strcmp(gras_ddt_parse_text,"enum")) {
			DEBUG0("This is an enum");
			type_modifier->is_enum = 1;

		} else if (gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_EMPTY) {
			DEBUG0("Pass space");

		} else {
			DEBUG1("Done with modifiers (got %s)",gras_ddt_parse_text);
			break;
		}

		gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();
		if((gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_WORD) &&
				(gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_STAR)) {
			DEBUG2("Done with modifiers (got %s,%d)",gras_ddt_parse_text,gras_ddt_parse_tok_num);
			break;
		}
	} while(1);
	XBT_OUT;
}

static void print_type_modifier(s_type_modifier_t tm) {
	int i;

	XBT_IN;
	if (tm.is_unsigned)             printf("(unsigned) ");
	if (tm.is_short)                printf("(short) ");
	for (i=0 ; i<tm.is_long ; i++)  printf("(long) ");

	if(tm.is_struct)                printf("(struct) ");
	if(tm.is_enum)                  printf("(enum) ");
	if(tm.is_union)                 printf("(union) ");

	for (i=0 ; i<tm.is_ref ; i++)   printf("(ref) ");
	XBT_OUT;
}

static void change_to_fixed_array(xbt_dynar_t dynar, long int size) {
	s_identifier_t former,array;
	memset(&array,0,sizeof(array));

	XBT_IN;
	xbt_dynar_pop(dynar,&former);
	array.type_name=(char*)xbt_malloc(strlen(former.type->name)+48);
	DEBUG2("Array specification (size=%ld, elm='%s'), change pushed type",
			size,former.type_name);
	sprintf(array.type_name,"%s%s%s%s[%ld]",
			(former.tm.is_unsigned?"u ":""),
			(former.tm.is_short?"s ":""),
			(former.tm.is_long?"l ":""),
			former.type_name,
			size);
	free(former.type_name);

	array.type = gras_datadesc_array_fixed(array.type_name, former.type, size); /* redeclaration are ignored */
	array.name = former.name;

	xbt_dynar_push(dynar,&array);
	XBT_OUT;
}
static void change_to_ref(xbt_dynar_t dynar) {
	s_identifier_t former,ref;
	memset(&ref,0,sizeof(ref));

	XBT_IN;
	xbt_dynar_pop(dynar,&former);
	ref.type_name=(char*)xbt_malloc(strlen(former.type->name)+2);
	DEBUG1("Ref specification (elm='%s'), change pushed type", former.type_name);
	sprintf(ref.type_name,"%s*",former.type_name);
	free(former.type_name);

	ref.type = gras_datadesc_ref(ref.type_name, former.type); /* redeclaration are ignored */
	ref.name = former.name;

	xbt_dynar_push(dynar,&ref);
	XBT_OUT;
}

static void change_to_ref_pop_array(xbt_dynar_t dynar) {
	s_identifier_t former,ref;
	memset(&ref,0,sizeof(ref));

	XBT_IN;
	xbt_dynar_pop(dynar,&former);
	ref.type = gras_datadesc_ref_pop_arr(former.type); /* redeclaration are ignored */
	ref.type_name = (char*)strdup(ref.type->name);
	ref.name = former.name;

	free(former.type_name);

	xbt_dynar_push(dynar,&ref);
	XBT_OUT;
}

static void change_to_dynar_of(xbt_dynar_t dynar,gras_datadesc_type_t subtype) {
	s_identifier_t former,ref;
	memset(&ref,0,sizeof(ref));

	XBT_IN;
	xbt_dynar_pop(dynar,&former);
	ref.type = gras_datadesc_dynar(subtype,NULL); /* redeclaration are ignored */
	ref.type_name = (char*)strdup(ref.type->name);
	ref.name = former.name;

	free(former.type_name);

	xbt_dynar_push(dynar,&ref);
	XBT_OUT;
}

static void change_to_matrix_of(xbt_dynar_t dynar,gras_datadesc_type_t subtype) {
	s_identifier_t former,ref;
	memset(&ref,0,sizeof(ref));

	XBT_IN;
	xbt_dynar_pop(dynar,&former);
	ref.type = gras_datadesc_matrix(subtype,NULL); /* redeclaration are ignored */
	ref.type_name = (char*)strdup(ref.type->name);
	ref.name = former.name;

	free(former.type_name);

	xbt_dynar_push(dynar,&ref);
	XBT_OUT;
}

static void add_free_f(xbt_dynar_t dynar,void_f_pvoid_t free_f) {
	s_identifier_t former,ref;
	memset(&ref,0,sizeof(ref));

	XBT_IN;
	xbt_dynar_pop(dynar,&former);
	memcpy(former.type->extra,free_f, sizeof(free_f));
	xbt_dynar_push(dynar,&former);
	XBT_OUT;
}

static void parse_statement(char	 *definition,
		xbt_dynar_t  identifiers,
		xbt_dynar_t  fields_to_push) {
	char buffname[512];

	s_identifier_t identifier;

	int expect_id_separator = 0;

	XBT_IN;
	memset(&identifier,0,sizeof(identifier));

	gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();
	if(gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_RA) {
		XBT_OUT;
		THROW0(mismatch_error,0,"End of the englobing structure or union");
	}

	if (XBT_LOG_ISENABLED(gras_ddt_parse,xbt_log_priority_debug)) {
		int colon_pos;
		for (colon_pos = gras_ddt_parse_col_pos;
		definition[colon_pos] != ';';
		colon_pos++);
		definition[colon_pos] = '\0';
		DEBUG3("Parse the statement \"%s%s;\" (col_pos=%d)",
				gras_ddt_parse_text,
				definition+gras_ddt_parse_col_pos,
				gras_ddt_parse_col_pos);
		definition[colon_pos] = ';';
	}

	if(gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_WORD)
		PARSE_ERROR1("Unparsable symbol: found a typeless statement (got '%s' instead)",
				gras_ddt_parse_text);

	/**** get the type modifier of this statement ****/
	parse_type_modifier(&identifier.tm);

	/*  FIXME: This does not detect recursive definitions at all? */
	if (identifier.tm.is_union || identifier.tm.is_enum || identifier.tm.is_struct)
		PARSE_ERROR0("Unimplemented feature: GRAS_DEFINE_TYPE cannot handle recursive type definition yet");

	/**** get the base type, giving "short a" the needed love ****/
	if (!identifier.tm.is_union &&
			!identifier.tm.is_enum  &&
			!identifier.tm.is_struct &&

			(identifier.tm.is_short || identifier.tm.is_long || identifier.tm.is_unsigned) &&

			strcmp(gras_ddt_parse_text,"char") &&
			strcmp(gras_ddt_parse_text,"float") &&
			strcmp(gras_ddt_parse_text,"double") &&
			strcmp(gras_ddt_parse_text,"int") ) {

		/* bastard user, they omited "int" ! */
		identifier.type_name=(char*)strdup("int");
		DEBUG0("the base type is 'int', which were omited (you vicious user)");
	} else {
		identifier.type_name=(char*)strdup(gras_ddt_parse_text);
		DEBUG1("the base type is '%s'",identifier.type_name);
		gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();
	}

	/**** build the base type for latter use ****/
	if (identifier.tm.is_union) {
		PARSE_ERROR0("Unimplemented feature: GRAS_DEFINE_TYPE cannot handle union yet (get callback from annotation?)");

	} else if (identifier.tm.is_enum) {
		PARSE_ERROR0("Unimplemented feature: GRAS_DEFINE_TYPE cannot handle enum yet");

	} else if (identifier.tm.is_struct) {
		sprintf(buffname,"struct %s",identifier.type_name);
		identifier.type = gras_datadesc_struct(buffname); /* Get created when does not exist */

	} else if (identifier.tm.is_unsigned) {
		if (!strcmp(identifier.type_name,"int")) {
			if (identifier.tm.is_long == 2) {
				identifier.type = gras_datadesc_by_name("unsigned long long int");
			} else if (identifier.tm.is_long) {
				identifier.type = gras_datadesc_by_name("unsigned long int");
			} else if (identifier.tm.is_short) {
				identifier.type = gras_datadesc_by_name("unsigned short int");
			} else {
				identifier.type = gras_datadesc_by_name("unsigned int");
			}

		} else if (!strcmp(identifier.type_name, "char")) {
			identifier.type = gras_datadesc_by_name("unsigned char");

		} else { /* impossible, gcc parses this shit before us */
			THROW_IMPOSSIBLE;
		}

	} else if (!strcmp(identifier.type_name, "float")) {
		/* no modificator allowed by gcc */
		identifier.type = gras_datadesc_by_name("float");

	} else if (!strcmp(identifier.type_name, "double")) {
		if (identifier.tm.is_long)
			PARSE_ERROR0("long double not portable and thus not handled");

		identifier.type = gras_datadesc_by_name("double");

	} else { /* signed integer elemental */
		if (!strcmp(identifier.type_name,"int")) {
			if (identifier.tm.is_long == 2) {
				identifier.type = gras_datadesc_by_name("signed long long int");
			} else if (identifier.tm.is_long) {
				identifier.type = gras_datadesc_by_name("signed long int");
			} else if (identifier.tm.is_short) {
				identifier.type = gras_datadesc_by_name("signed short int");
			} else {
				identifier.type = gras_datadesc_by_name("int");
			}

		} else if (!strcmp(identifier.type_name, "char")) {
			identifier.type = gras_datadesc_by_name("char");

		} else {
			DEBUG1("Base type is a constructed one (%s)",identifier.type_name);
			if (!strcmp(identifier.type_name,"xbt_matrix_t")) {
				identifier.tm.is_matrix = 1;
			} else if (!strcmp(identifier.type_name,"xbt_dynar_t")) {
				identifier.tm.is_dynar = 1;
			} else {
				identifier.type = gras_datadesc_by_name(identifier.type_name);
				if (!identifier.type)
					PARSE_ERROR1("Unknown base type '%s'",identifier.type_name);
			}
		}
	}
	/* Now identifier.type and identifier.name speak about the base type.
     Stars are not eaten unless 'int' was omitted.
     We will have to enhance it if we are in fact asked for array or reference.

     Dynars and matrices also need some extra love (prodiged as annotations)
	 */

	/**** look for the symbols of this type ****/
	for(expect_id_separator = 0;

	(/*(gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_EMPTY) && FIXME*/
			(gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_SEMI_COLON)) ;

	gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump()          ) {

		if(expect_id_separator) {
			if(gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_COLON) {
				expect_id_separator = 0;
				continue;

			} else if (gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_LB) {
				/* Handle fixed size arrays */
				gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();
				if (gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_RB) {
					PARSE_ERROR0("Unimplemented feature: GRAS_DEFINE_TYPE cannot deal with [] constructs (yet)");

				} else if (gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_WORD) {
					char *end;
					long int size=strtol(gras_ddt_parse_text, &end, 10);

					if (end == gras_ddt_parse_text || *end != '\0') {
						/* Not a number. Get the constant value, if any */
						int *storage=xbt_dict_get_or_null(gras_dd_constants,gras_ddt_parse_text);
						if (storage) {
							size = *storage;
						} else {
							PARSE_ERROR1("Unparsable size of array. Found '%s', expected number or known constant. Need to use gras_datadesc_set_const(), huh?",
									gras_ddt_parse_text);
						}
					}

					/* replace the previously pushed type to an array of it */
					change_to_fixed_array(identifiers,size);

					/* eat the closing bracket */
					gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();
					if (gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_RB)
						PARSE_ERROR0("Unparsable size of array");
					DEBUG1("Fixed size array, size=%ld",size);
					continue;
				} else {
					PARSE_ERROR0("Unparsable size of array");
				}
				/* End of fixed size arrays handling */

			} else if (gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_WORD) {
				/* Handle annotation */
				s_identifier_t array;
				char *keyname = NULL;
				char *keyval  = NULL;
				memset(&array,0,sizeof(array));
				if (strcmp(gras_ddt_parse_text,"GRAS_ANNOTE"))
					PARSE_ERROR1("Unparsable symbol: Expected 'GRAS_ANNOTE', got '%s'",gras_ddt_parse_text);

				gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();
				if (gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_LP)
					PARSE_ERROR1("Unparsable annotation: Expected parenthesis, got '%s'",gras_ddt_parse_text);

				while ( (gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump()) == GRAS_DDT_PARSE_TOKEN_EMPTY );

				if (gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_WORD)
					PARSE_ERROR1("Unparsable annotation: Expected key name, got '%s'",gras_ddt_parse_text);
				keyname = (char*)strdup(gras_ddt_parse_text);

				while ( (gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump()) == GRAS_DDT_PARSE_TOKEN_EMPTY );

				if (gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_COLON)
					PARSE_ERROR1("Unparsable annotation: expected ',' after the key name, got '%s'",gras_ddt_parse_text);

				while ( (gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump()) == GRAS_DDT_PARSE_TOKEN_EMPTY );

				/* get the value */

				if (gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_WORD)
					PARSE_ERROR1("Unparsable annotation: Expected key value, got '%s'",gras_ddt_parse_text);
				keyval = (char*)strdup(gras_ddt_parse_text);

				while ( (gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump()) == GRAS_DDT_PARSE_TOKEN_EMPTY );

				/* Done with parsing the annotation. Now deal with it by replacing previously pushed type with the right one */

				DEBUG2("Anotation: %s=%s",keyname,keyval);
				if (!strcmp(keyname,"size")) {
					free(keyname);
					if (!identifier.tm.is_ref)
						PARSE_ERROR0("Size annotation for a field not being a reference");
					identifier.tm.is_ref--;

					if (!strcmp(keyval,"1")) {
						change_to_ref(identifiers);
						free(keyval);
					} else {
						char *p;
						int fixed = 1;
						for (p = keyval; *p != '\0'; p++)
							if (! isdigit(*p) )
								fixed = 0;
						if (fixed) {
							change_to_fixed_array(identifiers,atoi(keyval));
							change_to_ref(identifiers);
							free(keyval);

						} else {
							change_to_ref_pop_array(identifiers);
							xbt_dynar_push(fields_to_push,&keyval);
						}
					}
				} else if (!strcmp(keyname,"subtype")) {
					gras_datadesc_type_t subtype = gras_datadesc_by_name(keyval);
					if (identifier.tm.is_matrix) {
						change_to_matrix_of(identifiers,subtype);
						identifier.tm.is_matrix = -1;
					} else if (identifier.tm.is_dynar) {
						change_to_dynar_of(identifiers,subtype);
						identifier.tm.is_dynar = -1;
					} else {
						PARSE_ERROR1("subtype annotation only accepted for dynars and matrices, but passed to '%s'",identifier.type_name);
					}
					free(keyval);
				} else if (!strcmp(keyname,"free_f")) {
					int *storage=xbt_dict_get_or_null(gras_dd_constants,keyval);
					if (!storage)
						PARSE_ERROR1("value for free_f annotation of field %s is not a known constant",identifier.name);
					if (identifier.tm.is_matrix == -1) {
						add_free_f(identifiers,*(void_f_pvoid_t*)storage);
						identifier.tm.is_matrix = 0;
					} else if (identifier.tm.is_dynar == -1) {
						add_free_f(identifiers,*(void_f_pvoid_t*)storage);
						identifier.tm.is_dynar = 0;
					} else {
						PARSE_ERROR1("free_f annotation only accepted for dynars and matrices which subtype is already declared (field %s)",
								identifier.name);
					}
					free(keyval);
				} else {
					free(keyval);
					PARSE_ERROR1("Unknown annotation type: '%s'",keyname);
				}

				/* Get all the multipliers */
				while (gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_STAR) {

					gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();

					if (gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_WORD)
						PARSE_ERROR1("Unparsable annotation: Expected field name after '*', got '%s'",gras_ddt_parse_text);

					keyval = xbt_malloc(strlen(gras_ddt_parse_text)+2);
					sprintf(keyval,"*%s",gras_ddt_parse_text);

					/* ask caller to push field as a multiplier */
					xbt_dynar_push(fields_to_push,&keyval);

					/* skip blanks after this block*/
					while ( (gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump())
							== GRAS_DDT_PARSE_TOKEN_EMPTY );
				}

				if (gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_RP)
					PARSE_ERROR1("Unparsable annotation: Expected parenthesis, got '%s'",
							gras_ddt_parse_text);

				continue;

				/* End of annotation handling */
			} else {
				PARSE_ERROR1("Unparsable symbol: Got '%s' instead of expected comma (',')",gras_ddt_parse_text);
			}
		} else if(gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_COLON) {
			PARSE_ERROR0("Unparsable symbol: Unexpected comma (',')");
		}

		if (gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_STAR) {
			identifier.tm.is_ref++; /* We indeed deal with multiple references with multiple annotations */
			continue;
		}

		/* found a symbol name. Build the type and push it to dynar */
		if(gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_WORD) {

			identifier.name=(char*)strdup(gras_ddt_parse_text);
			DEBUG1("Found the identifier \"%s\"",identifier.name);

			xbt_dynar_push(identifiers, &identifier);
			DEBUG1("Dynar_len=%lu",xbt_dynar_length(identifiers));
			expect_id_separator = 1;
			continue;
		}

		PARSE_ERROR0("Unparasable symbol (maybe a def struct in a def struct or a parser bug ;)");
	}

	if (identifier.tm.is_matrix>0)
		PARSE_ERROR0("xbt_matrix_t field without 'subtype' annotation");
	if (identifier.tm.is_dynar>0)
		PARSE_ERROR0("xbt_dynar_t field without 'subtype' annotation");

	XBT_OUT;
}

static gras_datadesc_type_t parse_struct(char *definition) {

	xbt_ex_t e;

	char buffname[32];
	static int anonymous_struct=0;

	xbt_dynar_t identifiers;
	s_identifier_t field;
	unsigned int iter;
	int done;

	xbt_dynar_t fields_to_push;
	char *name;

	gras_datadesc_type_t struct_type;

	XBT_IN;
	identifiers = xbt_dynar_new(sizeof(s_identifier_t),NULL);
	fields_to_push = xbt_dynar_new(sizeof(char*),NULL);

	/* Create the struct descriptor */
	if (gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_WORD) {
		struct_type = gras_datadesc_struct(gras_ddt_parse_text);
		VERB1("Parse the struct '%s'", gras_ddt_parse_text);
		gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();
	} else {
		sprintf(buffname,"anonymous struct %d",anonymous_struct++);
		VERB1("Parse the anonymous struct nb %d", anonymous_struct);
		struct_type = gras_datadesc_struct(buffname);
	}

	if (gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_LA)
		PARSE_ERROR1("Unparasable symbol: Expecting struct definition, but got %s instead of '{'",
				gras_ddt_parse_text);

	/* Parse the identifiers */
	done = 0;
	do {
		TRY {
			parse_statement(definition,identifiers,fields_to_push);
		} CATCH(e) {
			if (e.category != mismatch_error)
				RETHROW;
			xbt_ex_free(e);
			done = 1;
		}

		DEBUG1("This statement contained %lu identifiers",xbt_dynar_length(identifiers));
		/* append the identifiers we've found */
		xbt_dynar_foreach(identifiers,iter, field) {
			if (field.tm.is_ref)
				PARSE_ERROR2("Not enough GRAS_ANNOTATE to deal with all dereferencing levels of %s (%d '*' left)",
						field.name,field.tm.is_ref);

			VERB2("Append field '%s' to %p",field.name, (void*)struct_type);
			gras_datadesc_struct_append(struct_type, field.name, field.type);
			free(field.name);
			free(field.type_name);

		}
		xbt_dynar_reset(identifiers);
		DEBUG1("struct_type=%p",(void*)struct_type);

		/* Make sure that all fields declaring a size push it into the cbps */
		xbt_dynar_foreach(fields_to_push,iter, name) {
			DEBUG1("struct_type=%p",(void*)struct_type);
			if (name[0] == '*') {
				VERB2("Push field '%s' as a multiplier into size stack of %p",
						name+1, (void*)struct_type);
				gras_datadesc_cb_field_push_multiplier(struct_type, name+1);
			} else {
				VERB2("Push field '%s' into size stack of %p",
						name, (void*)struct_type);
				gras_datadesc_cb_field_push(struct_type, name);
			}
			free(name);
		}
		xbt_dynar_reset(fields_to_push);
	} while (!done);
	gras_datadesc_struct_close(struct_type);

	/* terminates */
	if (gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_RA)
		PARSE_ERROR1("Unparasable symbol: Expected '}' at the end of struct definition, got '%s'",
				gras_ddt_parse_text);

	gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();

	xbt_dynar_free(&identifiers);
	xbt_dynar_free(&fields_to_push);
	XBT_OUT;
	return struct_type;
}

static gras_datadesc_type_t parse_typedef(char *definition) {

	s_type_modifier_t tm;

	gras_datadesc_type_t struct_desc=NULL;
	gras_datadesc_type_t typedef_desc=NULL;

	XBT_IN;
	memset(&tm,0,sizeof(tm));

	/* get the aliased type */
	parse_type_modifier(&tm);

	if (tm.is_struct) {
		struct_desc = parse_struct(definition);
	}

	parse_type_modifier(&tm);

	if (tm.is_ref)
		PARSE_ERROR0("GRAS_DEFINE_TYPE cannot handle reference without annotation");

	/* get the aliasing name */
	if (gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_WORD)
		PARSE_ERROR1("Unparsable typedef: Expected the alias name, and got '%s'",
				gras_ddt_parse_text);

	/* (FIXME: should) build the alias */
	PARSE_ERROR0("Unimplemented feature: GRAS_DEFINE_TYPE cannot handle typedef yet");

	XBT_OUT;
	return typedef_desc;
}


/**
 * gras_datadesc_parse:
 *
 * Create a datadescription from the result of parsing the C type description
 */
gras_datadesc_type_t
gras_datadesc_parse(const char            *name,
		const char            *C_statement) {

	gras_datadesc_type_t res=NULL;
	char *definition;
	int semicolon_count=0;
	int def_count,C_count;

	XBT_IN;
	/* reput the \n in place for debug */
	for (C_count=0; C_statement[C_count] != '\0'; C_count++)
		if (C_statement[C_count] == ';' || C_statement[C_count] == '{')
			semicolon_count++;
	definition = (char*)xbt_malloc(C_count + semicolon_count + 1);
	for (C_count=0,def_count=0; C_statement[C_count] != '\0'; C_count++) {
		definition[def_count++] = C_statement[C_count];
		if (C_statement[C_count] == ';' || C_statement[C_count] == '{') {
			definition[def_count++] = '\n';
		}
	}
	definition[def_count] = '\0';

	/* init */
	VERB2("_gras_ddt_type_parse(%s) -> %d chars",definition, def_count);
	gras_ddt_parse_pointer_string_init(definition);

	/* Do I have a typedef, or a raw struct ?*/
	gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();

	if ((gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_WORD) && (!strcmp(gras_ddt_parse_text,"struct"))) {
		gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();
		res = parse_struct(definition);

	} else if ((gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_WORD) && (!strcmp(gras_ddt_parse_text,"typedef"))) {
		gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();
		res = parse_typedef(definition);

	} else {
		ERROR1("Failed to parse the following symbol (not a struct neither a typedef) :\n%s",definition);
		xbt_abort();
	}

	gras_ddt_parse_pointer_string_close();
	VERB0("end of _gras_ddt_type_parse()");
	free(definition);
	/* register it under the name provided as symbol */
	if (strcmp(res->name,name)) {
		ERROR2("In GRAS_DEFINE_TYPE, the provided symbol (here %s) must be the C type name (here %s)",
				name,res->name);
		xbt_abort();
	}
	gras_ddt_parse_lex_destroy();
	XBT_OUT;
	return res;
}

xbt_dict_t gras_dd_constants;
/** \brief Declare a constant to the parsing mecanism. See the "\#define and fixed size array" section */
void gras_datadesc_set_const(const char*name, int value) {
	int *stored = xbt_new(int, 1);
	*stored=value;

	xbt_dict_set(gras_dd_constants,name, stored, xbt_free_f);
}
