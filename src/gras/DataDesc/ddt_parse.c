/* $Id$ */

/* DataDesc/ddt_parse.c -- automatic parsing of data structures */

/* Authors: Arnaud Legrand, Martin Quinson            */
/* Copyright (C) 2003, 2004 Martin Quinson.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "DataDesc/datadesc_private.h"
#include "DataDesc/ddt_parse.yy.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(parse,datadesc);

typedef struct s_type_modifier{
  short is_unsigned;
  short is_short;
  short is_long;

  short is_struct;
  short is_union;
  short is_enum;

  short is_ref;
} type_modifier_t;

typedef struct s_field {
  gras_datadesc_type_t *type;
  char *type_name;
  char *name;
} identifier_t;
 
extern char *gras_ddt_parse_text; /* text being considered in the parser */

/* prototypes */
static void parse_type_modifier(type_modifier_t 	*type_modifier);
static void print_type_modifier(type_modifier_t		 type_modifier);

static gras_error_t parse_statement(char		*definition,
				    gras_dynar_t 	**dynar);
static gras_datadesc_type_t * parse_struct(char	*definition);
static gras_datadesc_type_t * parse_typedef(char	*definition);

/* local functions */
static void parse_type_modifier(type_modifier_t 	*type_modifier)  {
  DEBUG0("Parse the modifiers");
  do {
    if (gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_STAR) {
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
      
    } else {
      DEBUG1("Done with modifiers (got %s)",gras_ddt_parse_text);
      break;
    }

    gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();
    if((gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_WORD) && 
       (gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_STAR)) {
      DEBUG1("Done with modifiers (got %s)",gras_ddt_parse_text);      
      break;
    }
  } while(1);
}

static void print_type_modifier(type_modifier_t tm) {
  int i;

  if (tm.is_unsigned)             printf("(unsigned) ");
  if (tm.is_short)                printf("(short) ");
  for (i=0 ; i<tm.is_long ; i++)  printf("(long) ");

  if(tm.is_struct)                printf("(struct) ");
  if(tm.is_enum)                  printf("(enum) ");
  if(tm.is_union)                 printf("(union) ");

  for (i=0 ; i<tm.is_ref ; i++)   printf("(ref) ");
}

static gras_error_t parse_statement(char		*definition,
				    gras_dynar_t 	**dynar) {
  gras_error_t errcode;
  char buffname[512];

  identifier_t identifier;
  type_modifier_t tm;

  int starred = 0;
  int expect_id_separator = 0;

  gras_dynar_reset(*dynar);
  memset(&identifier,0,sizeof(identifier));
  memset(&tm,0,sizeof(tm));
    
  gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();
  if(gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_RP) {
    return mismatch_error; /* end of the englobing structure or union */
  }
  
  if (GRAS_LOG_ISENABLED(parse,gras_log_priority_debug)) {
    int colon_pos;
    for (colon_pos = gras_ddt_parse_col_pos;
	 definition[colon_pos] != ';';
	 colon_pos++);
    definition[colon_pos] = '\0';
    DEBUG2("Parse the statement \"%s%s;\"",
	   gras_ddt_parse_text,
	   definition+gras_ddt_parse_col_pos);
    definition[colon_pos] = ';';
  }

  if(gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_WORD) {
    ERROR2("Unparsable symbol: found a typeless statement (got '%s' instead). Definition was:\n%s",
	   gras_ddt_parse_text, definition);
    gras_abort();
  }

  /**** get the type modifier of this statement ****/
  parse_type_modifier(&tm);

  /*  FIXME: This does not detect recursive definitions at all? */
  if (tm.is_union || tm.is_enum || tm.is_struct) {
    ERROR1("Cannot handle recursive type definition yet. Definition was:\n%s",
	   definition);
    gras_abort();
  }

  /**** get the base type, giving "short a" the needed love ****/
  if (!tm.is_union &&
      !tm.is_enum  && 
      !tm.is_struct &&

      (tm.is_short || tm.is_long || tm.is_unsigned) &&

      strcmp(gras_ddt_parse_text,"char") && 
      strcmp(gras_ddt_parse_text,"float") && 
      strcmp(gras_ddt_parse_text,"double") && 
      strcmp(gras_ddt_parse_text,"int") ) {

    /* bastard user, they omited "int" ! */
    identifier.type_name=strdup("int");
    DEBUG0("the base type is 'int', which were omited");
  } else {
    identifier.type_name=strdup(gras_ddt_parse_text);
    DEBUG1("the base type is '%s'",identifier.type_name);
    gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump(); 
  }

  /**** build the base type for latter use ****/
  if (tm.is_union) {
    ERROR1("Cannot handle union yet (need annotation to get the callback). Definition was:\n%s",
	    definition);
    gras_abort();

  } else if (tm.is_enum) {
    ERROR1("Cannot handle enum yet. Definition was:\n%s",
	   definition);
    gras_abort();

  } else if (tm.is_struct) {
    sprintf(buffname,"struct %s",identifier.type_name);
    identifier.type = gras_datadesc_by_name(buffname);
    if (!identifier.type) {
      TRY(gras_datadesc_declare_struct(buffname,&identifier.type));
    }

  } else if (tm.is_unsigned) {
    if (!strcmp(identifier.type_name,"int")) {
      if (tm.is_long == 2) {
	identifier.type = gras_datadesc_by_name("unsigned long long int");
      } else if (tm.is_long) {
	identifier.type = gras_datadesc_by_name("unsigned long int");
      } else if (tm.is_short) {
	identifier.type = gras_datadesc_by_name("unsigned short int");
      } else {
	identifier.type = gras_datadesc_by_name("unsigned int");
      }

    } else if (!strcmp(identifier.type_name, "char")) {
      identifier.type = gras_datadesc_by_name("unsigned char");

    } else { /* impossible, gcc parses this shit before us */
      RAISE_IMPOSSIBLE;
    }
    
  } else if (!strcmp(identifier.type_name, "float")) {
    /* no modificator allowed by gcc */
    identifier.type = gras_datadesc_by_name("float");

  } else if (!strcmp(identifier.type_name, "double")) {
    if (tm.is_long) {
      ERROR0("long double not portable and thus not handled");
      gras_abort();
    }
    identifier.type = gras_datadesc_by_name("double");

  } else { /* signed integer elemental */
    if (!strcmp(identifier.type_name,"int")) {
      if (tm.is_long == 2) {
	identifier.type = gras_datadesc_by_name("signed long long int");
      } else if (tm.is_long) {
	identifier.type = gras_datadesc_by_name("signed long int");
      } else if (tm.is_short) {
	identifier.type = gras_datadesc_by_name("signed short int");
      } else {
	identifier.type = gras_datadesc_by_name("int");
      }

    } else if (!strcmp(identifier.type_name, "char")) {
      identifier.type = gras_datadesc_by_name("char");

    } else { /* impossible */
      ERROR3("The Impossible did happen at %d:%d of :\n%s",
	     gras_ddt_parse_line_pos,gras_ddt_parse_char_pos,definition);
      gras_abort();
    }
    
  } 

  if (tm.is_ref) {
    ERROR1("Cannot handle references yet (need annotations), sorry. Definition was:\n%s",
	   definition);
    gras_abort();
    /* Should build ref on the current identifier.type (beware of int****) */
  }
  
  /**** look for the symbols of this type ****/
  for(expect_id_separator = 0;

      ((gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_EMPTY) &&
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
	  ERROR3("Cannot dynamically sized array at %d:%d of %s",
		 gras_ddt_parse_line_pos,gras_ddt_parse_char_pos,
		 definition);
	  gras_abort();
	} else if (gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_WORD) {
	  char *end;
	  long int size=strtol(gras_ddt_parse_text, &end, 10);
	  identifier_t array;

	  if (end == gras_ddt_parse_text ||
	      *end != '\0') {
	    ERROR3("Unparsable size of array at %d:%d of %s",
		   gras_ddt_parse_line_pos,gras_ddt_parse_char_pos,
		   definition);
	    gras_abort();
	  }
	  /* replace the previously pushed type to an array of it */
	  gras_dynar_pop(*dynar,&identifier.type);
	  array.type_name=malloc(strlen(identifier.type->name)+20);
	  DEBUG2("Array specification (size=%ld, elm='%s'), change pushed type",
		 size,identifier.type_name);
	  sprintf(array.type_name,"%s[%ld]",identifier.type_name,size);
	  free(identifier.type_name);
	  array.type = gras_datadesc_by_name(array.type_name);
	  if (array.type==NULL) {
	    TRY(gras_datadesc_declare_array_fixed(array.type_name,
						  identifier.type,
						  size, &array.type));
	  }
	  array.name = identifier.name;
	  TRY(gras_dynar_push(*dynar,&array));

	  /* eat the closing bracket */
	  gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();
	  if (gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_RB) {
	    ERROR3("Unparsable size of array at %d:%d of %s",
		   gras_ddt_parse_line_pos,gras_ddt_parse_char_pos,
		   definition);
	    gras_abort();
	  }
	DEBUG1("Fixed size array, size=%d",size);
	continue;
	} else {
	  ERROR3("Unparsable size of array at %d:%d of %s",
		 gras_ddt_parse_line_pos,gras_ddt_parse_char_pos,
		 definition);
	  gras_abort();
	}
      } else {
	ERROR2("Unparsable symbol: Expected a comma (','), got '%s' instead. Definition was:\n%s",
		gras_ddt_parse_text, definition);
	gras_abort();
      }
    } else if(gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_COLON) {
      ERROR1("Unparsable symbol: Unexpected comma (','). Definition was:\n%s",
	     definition);
      gras_abort();
    }

    if(gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_STAR) {
      starred = 1;
    }

    /* found a symbol name. Build the type and push it to dynar */
    if(gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_WORD) {
      if (starred) {
	/* FIXME: Build a ref or array on the base type */
	ERROR1("Cannot handle references yet (need annotations), sorry. Definition was:\n%s",
	       definition);
	gras_abort();
      }
      identifier.name=strdup(gras_ddt_parse_text);
      DEBUG1("Found the identifier \"%s\"",identifier.name);
      
      TRY(gras_dynar_push(*dynar, &identifier));
      starred = 0;
      expect_id_separator = 1;
      continue;
    } 

    ERROR3("Unparasable symbol (maybe a def struct in a def struct or so) at %d:%d of\n%s",
	   gras_ddt_parse_line_pos,gras_ddt_parse_char_pos,
	   definition);
    gras_abort();
  }

  DEBUG0("End of this statement");
  return no_error;
}

static gras_datadesc_type_t *parse_struct(char *definition) {

  gras_error_t errcode;
  char buffname[32];
  static int anonymous_struct=0;

  gras_dynar_t *fields;

  identifier_t field;
  int i;

  gras_datadesc_type_t *struct_type;

  errcode=gras_dynar_new(&fields,sizeof(identifier_t),NULL);
  if (errcode != no_error) 
    return NULL;

  /* Create the struct descriptor */
  if (gras_ddt_parse_tok_num == GRAS_DDT_PARSE_TOKEN_WORD) {
    TRYFAIL(gras_datadesc_declare_struct(gras_ddt_parse_text,&struct_type));
    DEBUG1("Parse the struct '%s'", gras_ddt_parse_text);
    gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();
  } else {
    sprintf(buffname,"anonymous struct %d",anonymous_struct++); 
    DEBUG1("Parse the anonymous struct nb %d", anonymous_struct);
    TRYFAIL(gras_datadesc_declare_struct(buffname,&struct_type));
  }

  if (gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_LP) {
    ERROR2("Unparasable symbol: Expecting struct definition, but got %s instead of '{'. The definition was:\n%s",
	    gras_ddt_parse_text,definition);
    gras_abort();
  }

  /* Parse the fields */
  for (errcode=parse_statement(definition,&fields);
       errcode == no_error                            ;
       errcode=parse_statement(definition,&fields)) {
    
    DEBUG1("This statement contained %d fields",gras_dynar_length(fields));
    gras_dynar_foreach(fields,i, field) {
      DEBUG1("Append field %s",field.name);      
      TRYFAIL(gras_datadesc_declare_struct_append(struct_type,field.name,
						  field.type));
      free(field.name);
      free(field.type_name);
    }
  }
  gras_datadesc_declare_struct_close(struct_type);
  if (errcode != mismatch_error)
    return NULL; /* FIXME: LEAK! */

  /* terminates */
  if (gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_RP) {
    ERROR2("Unparasable symbol: Expected '}' at the end of struct definition, got '%s'. The definition was:\n%s",
	   gras_ddt_parse_text,definition);
    gras_abort();
  } 

  gras_ddt_parse_tok_num = gras_ddt_parse_lex_n_dump();

  gras_dynar_free(fields);
  return struct_type;
}

static gras_datadesc_type_t * parse_typedef(char *definition) {

  type_modifier_t tm;

  gras_datadesc_type_t *struct_desc=NULL;
  gras_datadesc_type_t *typedef_desc=NULL;

  memset(&tm,0,sizeof(tm));

  /* get the aliased type */
  parse_type_modifier(&tm);

  if (tm.is_struct) {
    struct_desc = parse_struct(definition);
  }

  parse_type_modifier(&tm);

  if (tm.is_ref) {
    ERROR1("Cannot handle reference without annotation. Definition was:\n%s",
	   definition);
    gras_abort();
  }    

  /* get the aliasing name */
  if (gras_ddt_parse_tok_num != GRAS_DDT_PARSE_TOKEN_WORD) {
    ERROR2("Unparsable typedef: Expected the alias name, and got '%s'.\n%s",
	   gras_ddt_parse_text,definition);
    gras_abort();
  }
  
  /* (FIXME: should) build the alias */
  ERROR1("Cannot handle typedef yet. Definition was: \n%s",definition);
  gras_abort();

  //  identifier.type=gras_ddt_type_new_typedef(bag, NULL, strdup(gras_ddt_parse_text) );
  
  return typedef_desc;
}


/**
 * gras_datadesc_declare_parse:
 *
 * Create a datadescription from the result of parsing the C type description
 */
gras_datadesc_type_t *
gras_datadesc_parse(const char            *name,
		    const char            *C_statement) {

  gras_datadesc_type_t * res=NULL;
  char *definition;
  int semicolon_count=0;
  int def_count,C_count;
  /* reput the \n in place for debug */
  for (C_count=0; C_statement[C_count] != '\0'; C_count++)
    if (C_statement[C_count] == ';' || C_statement[C_count] == '{')
      semicolon_count++;
  definition = malloc(C_count + semicolon_count + 1);
  for (C_count=0,def_count=0; C_statement[C_count] != '\0'; C_count++) {
    definition[def_count++] = C_statement[C_count];
    if (C_statement[C_count] == ';' || C_statement[C_count] == '{') {
      definition[def_count++] = '\n';
    }
  }
  definition[def_count] = '\0';

  /* init */ 
  VERB1("_gras_ddt_type_parse(%s)",definition);
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
    gras_abort();
  }

  gras_ddt_parse_pointer_string_close();
  VERB0("end of _gras_ddt_type_parse()");
  free(definition);
  /* register it under the name provided as symbol */
  if (strcmp(res->name,name)) {
    ERROR2("In GRAS_DEFINE_TYPE, the provided symbol (here %s) must be the C type name (here %s)",
	   name,res->name);
    gras_abort();
  }    
  return res;
}


