/* $Id$ */

/* gs/parse.c -- automatic parsing of data structures */

/* Authors: Arnaud Legrand, Martin Quinson            */

#include "gs/gs_private.h"
#include "gs/parse.yy.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(NDR_parse,NDR);

typedef struct s_type_modifier{
  short is_unsigned;
  short is_short;
  short is_long;

  short is_struct;
  short is_union;
  short is_enum;

  short is_ref;
} type_modifier_t;
 
extern char *gs_parse_text;

/* prototypes */
static void parse_type_modifier(type_modifier_t 	*type_modifier);
static void print_type_modifier(type_modifier_t		 type_modifier);

static gras_error_t parse_statement(gras_type_bag_t	*p_bag,
				    const char		*definition,
				    gras_dynar_t 	**dynar);
static gras_type_t * parse_struct(gras_type_bag_t	*p_bag,
				  const char		*definition);
static gras_type_t * parse_typedef(gras_type_bag_t	*p_bag,
				   const char		*definition);

/* local functions */
static void parse_type_modifier(type_modifier_t 	*type_modifier)  {
  do {
    if (gs_parse_tok_num == GS_PARSE_TOKEN_STAR) {
      DEBUG0("This is a reference");
      type_modifier->is_ref++;

    } else if (!strcmp(gs_parse_text,"unsigned")) {
      DEBUG0("This is an unsigned");
      type_modifier->is_unsigned = 1;

    } else if (!strcmp(gs_parse_text,"short")) {
      DEBUG0("This is short");
      type_modifier->is_short = 1;

    } else if (!strcmp(gs_parse_text,"long")) {
      DEBUG0("This is long");
      type_modifier->is_long++; /* "long long" needs love */

    } else if (!strcmp(gs_parse_text,"struct")) {
      DEBUG0("This is a struct");
      type_modifier->is_struct = 1;

    } else if (!strcmp(gs_parse_text,"union")) {
      DEBUG0("This is an union");
      type_modifier->is_union = 1;

    } else if (!strcmp(gs_parse_text,"enum")) {
      DEBUG0("This is an enum");
      type_modifier->is_enum = 1;

    } else {
      break;
    }
    

    gs_parse_tok_num = gs_parse_lex_n_dump();
    if((gs_parse_tok_num != GS_PARSE_TOKEN_WORD) && 
       (gs_parse_tok_num != GS_PARSE_TOKEN_STAR)) 
      break;
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

/* FIXME: Missing function. Do not free the dynar... */
#define gs_type_copy(a) a

static gras_error_t parse_statement(gras_type_bag_t	*bag,
				    const char		*definition,
				    gras_dynar_t 	**dynar) {
  gras_error_t errcode;
  char buffname[512];

  char *base_type = NULL;
  type_modifier_t tm;

  gras_type_t *res;

  int starred = 0;
  int expect_a_colon = 0;

  gras_dynar_reset(*dynar);
  memset(&tm,0,sizeof(tm));
    
  gs_parse_tok_num = gs_parse_lex_n_dump();
  if(gs_parse_tok_num == GS_PARSE_TOKEN_RP) {
    return mismatch_error; /* end of the englobing structure or union */
  }
  DEBUG0("Parse a new statement.");


  if(gs_parse_tok_num != GS_PARSE_TOKEN_WORD) {
    fprintf(stderr,
	    "Unparsable symbol: found a typeless statement (got '%s' instead). Definition was:\n%s\n",
	    gs_parse_text, definition);
    abort();
  }

  /**** get the type modifier of this statement ****/
  parse_type_modifier(&tm);

  /*  FIXME: This does not detect recursive definitions at all
  if (tm.is_union || tm.is_enum || tm.is_struct) {
    fprintf(stderr,
	    "Cannot handle recursive type definition yet. Definition was:\n%s\n",
	    definition);
    abort();
    }*/

  /**** get the base type, giving "short a" the needed love ****/
  if (!tm.is_union &&
      !tm.is_enum  && 
      !tm.is_struct &&

      (tm.is_short || tm.is_long || tm.is_unsigned) &&

      strcmp(gs_parse_text,"char") && 
      strcmp(gs_parse_text,"float") && 
      strcmp(gs_parse_text,"double") && 
      strcmp(gs_parse_text,"int") ) {

    /* bastard user, they omited "int" ! */
    base_type=strdup("int");
    DEBUG0("the base type is 'int', which were omited");
  } else {
    base_type=strdup(gs_parse_text);
    DEBUG1("the base type is '%s'",base_type);
    gs_parse_tok_num = gs_parse_lex_n_dump(); 
  }

  /**** build the base type for latter use ****/
  if (tm.is_union) {
    fprintf(stderr,
	    "Cannot handle union yet (need annotation to get the callback), sorry. Definition was:\n%s\n",
	    definition);
    abort();

    /* FIXME 
    sprintf(buffname,"union %s",base_type);
    res = bag->bag_ops->get_type_by_name(bag, NULL, buffname);
    if (!res) {
      res = gs_type_new_union(bag,NULL,buffname);
    }
    */

  } else if (tm.is_enum) {
    fprintf(stderr,
	    "Cannot handle enum yet (need grassouillet's love), sorry. Definition was:\n%s\n",
	    definition);
    abort();
    /* FIXME
    sprintf(buffname,"enum %s",base_type);
    res = bag->bag_ops->get_type_by_name(bag, NULL, bufname);
    if (!res) {
      res = gs_type_new_enum(bag,connection,buffname);
    }
    */

  } else if (tm.is_struct) {
    sprintf(buffname,"struct %s",base_type);
    res = bag->bag_ops->get_type_by_name(bag, NULL, buffname);
    if (!res) {
      res = gs_type_new_struct(bag,NULL,buffname);
    }

    /* Let's code like Alvin ;) */
#define gs_parse_get_or_create(name,func)               \
   (bag->bag_ops->get_type_by_name(bag, NULL, #name) ?  \
    bag->bag_ops->get_type_by_name(bag, NULL, #name) :  \
    gs_type_new_##func##_elemental(bag, NULL,           \
                                   #name, sizeof(name)) \
   )

  } else if (tm.is_unsigned) {
    if (!strcmp(base_type,"int")) {
      if (tm.is_long == 2) {
	res = gs_parse_get_or_create(unsigned long long int,unsigned_integer);
      } else if (tm.is_long) {
	res = gs_parse_get_or_create(unsigned long int,unsigned_integer);
      } else if (tm.is_short) {
	res = gs_parse_get_or_create(unsigned short int,unsigned_integer);
      } else {
	res = gs_parse_get_or_create(unsigned int,unsigned_integer);
      }

    } else if (!strcmp(base_type, "char")) {
      res = gs_parse_get_or_create(unsigned char,unsigned_integer);

    } else { /* impossible, gcc parses this shit before us */
      abort(); 
    }
    
  } else if (!strcmp(base_type, "float")) {
    /* no modificator allowed by gcc */
    res = gs_parse_get_or_create(float,floating_point);

  } else { /* signed integer elemental */
    if (!strcmp(base_type,"int")) {
      if (tm.is_long == 2) {
	res = gs_parse_get_or_create(signed long long int,signed_integer);
      } else if (tm.is_long) {
	res = gs_parse_get_or_create(signed long int,signed_integer);
      } else if (tm.is_short) {
	res = gs_parse_get_or_create(signed short int,signed_integer);
      } else {
	res = gs_parse_get_or_create(int,unsigned_integer);
      }

    } else if (!strcmp(base_type, "char")) {
      res = gs_parse_get_or_create(char,unsigned_integer);

    } else { /* impossible */
      abort(); 
    }

  } 

  if (tm.is_ref) {
    fprintf(stderr,
	    "Cannot handle references yet (need annotations), sorry. Definition was:\n%s\n",
	    definition);
    abort();
    /* Should build ref on the current res (beware of int****) */
  }

  /**** look for the symbols of this type ****/
  expect_a_colon = 0;
  for(          /* no initialization */                 ; 

      ((gs_parse_tok_num != GS_PARSE_TOKEN_EMPTY) &&
       (gs_parse_tok_num != GS_PARSE_TOKEN_SEMI_COLON)) ; 

      gs_parse_tok_num = gs_parse_lex_n_dump()          ) {   

    if(expect_a_colon) {
      if(gs_parse_tok_num == GS_PARSE_TOKEN_COLON) {
        expect_a_colon = 0;
        continue;
      } else {
	fprintf(stderr,
		"Unparsable symbol: Expected a comma (','), got '%s' instead. Definition was:\n%s\n",
		gs_parse_text, definition);
	abort();
      }
    } else if(gs_parse_tok_num == GS_PARSE_TOKEN_COLON) {
      fprintf(stderr,
	      "Unparsable symbol: Unexpected comma (','). Definition was:\n%s\n",
	      definition);
      abort();
    }

    if(gs_parse_tok_num == GS_PARSE_TOKEN_STAR) {
      starred = 1;
    }

    /* found a symbol name. Build the type and push it to dynar */
    if(gs_parse_tok_num == GS_PARSE_TOKEN_WORD) {
      if (starred) {
	/* FIXME: Build a ref or array on the base type */
	fprintf(stderr,
		"Cannot handle references yet (need annotations), sorry. Definition was:\n%s\n",
		definition);
	abort();
      }
      DEBUG1("Encountered the variable (field?) %s",gs_parse_text);

      TRY(gras_dynar_push(*dynar, &gs_type_copy(base_type)));
      starred = 0;
      expect_a_colon = 1;
      continue;
    } 

    fprintf(stderr,
	    "Unparasable symbol: Maybe a def struct in a def struct or so. The definition was:\n%s\n",
	    definition);
    abort();
  }

  DEBUG0("End of this statement");
  return no_error;
}

static gras_type_t *parse_struct(gras_type_bag_t	*bag,
				 const char		*definition) {

  gras_error_t errcode;
  char buffname[32];
  static int anonymous_struct=0;

  gras_dynar_t *fields;

  gras_type_t *field;
  int i;

  gras_type_t *struct_type;

  errcode=gras_dynar_new(&fields,sizeof(gras_type_t*),NULL);
  if (errcode != no_error) 
    return NULL;
  /* FIXME: the dynar content leaks because there is no gs_type_free. 
     But that's not a problem because each member are the same element since there is no gs_type_copy ;)
  */

  /* Create the struct descriptor */
  if (gs_parse_tok_num == GS_PARSE_TOKEN_WORD) {
    struct_type = gs_type_new_struct(bag,NULL, gs_parse_text);
    DEBUG1("Parse the struct '%s'", gs_parse_text);
    gs_parse_tok_num = gs_parse_lex_n_dump();
  } else {
    sprintf(buffname,"anonymous struct %d",anonymous_struct++); 
    DEBUG1("Parse the anonymous struct nb %d", anonymous_struct);
    struct_type = gs_type_new_struct(bag,NULL,buffname);
  }

  if (gs_parse_tok_num != GS_PARSE_TOKEN_LP) {
    fprintf(stderr,
	    "Unparasable symbol: I looked for a struct definition, but got %s instead of '{'. The definition was:\n%s\n",
	    gs_parse_text,definition);
    abort();
  }

  /* Parse the fields */
  for (errcode=parse_statement(bag,definition,&fields);
       errcode == no_error                            ;
       errcode=parse_statement(bag,definition,&fields)) {
    
    DEBUG1("This statement contained %d fields",gras_dynar_length(fields));
    gras_dynar_foreach(fields,i, field) {
      char fname[128];
      static int field_count=0;
      sprintf(fname,"%d",field_count++);
      DEBUG0("Append a field");      
      gs_type_struct_append_field(struct_type,fname,field);
    }
  }
  if (errcode != mismatch_error)
    return NULL; /* FIXME: LEAK! */

  /* terminates */
  if (gs_parse_tok_num != GS_PARSE_TOKEN_RP) {
    fprintf(stderr,
	    "Unparasable symbol: Expected '}' at the end of struct definition, got '%s'. The definition was:\n%s\n",
	    gs_parse_text,definition);
    abort();
  } 

  gs_parse_tok_num = gs_parse_lex_n_dump();

  gras_dynar_free(fields);
  return struct_type;
}

static gras_type_t * parse_typedef(gras_type_bag_t	*bag,
				   const char		*definition) {

  type_modifier_t tm;

  gras_type_t *struct_desc=NULL;
  gras_type_t *typedef_desc=NULL;

  memset(&tm,0,sizeof(tm));

  /* get the aliased type */
  parse_type_modifier(&tm);

  if (tm.is_struct) {
    struct_desc = parse_struct(bag,definition);
  }

  parse_type_modifier(&tm);

  if (tm.is_ref) {
    fprintf(stderr,
	    "Cannot handle reference without annotation. Definition was:\n%s\n",
	    definition);
    abort();
  }    

  /* get the aliasing name */
  if (gs_parse_tok_num != GS_PARSE_TOKEN_WORD) {
    fprintf(stderr,
	    "Unparsable typedef: Expected the alias name, and got '%s'.\n%s\n",
	    gs_parse_text,definition);
    abort();
  }
  
  /* (FIXME: should) build the alias */
  fprintf(stderr, "Cannot handle typedef yet (need love from grassouillet), sorry. Definition was: \n%s\n",definition);
  abort();

  //  res=gs_type_new_typedef(bag, NULL, strdup(gs_parse_text) );
  
  return typedef_desc;
}

/* main function */
gras_type_t *
_gs_type_parse(gras_type_bag_t	*bag,
	       const char	*definition) {

  gras_type_t * res=NULL;

  /* init */ 
  DEBUG1("_gs_type_parse(%s)",definition);
  gs_parse_pointer_string_init(definition);

  /* Do I have a typedef, or a raw struct ?*/
  gs_parse_tok_num = gs_parse_lex_n_dump();
  
  if ((gs_parse_tok_num == GS_PARSE_TOKEN_WORD) && (!strcmp(gs_parse_text,"struct"))) {
    gs_parse_tok_num = gs_parse_lex_n_dump();
    res = parse_struct(bag,definition);
      
  } else if ((gs_parse_tok_num == GS_PARSE_TOKEN_WORD) && (!strcmp(gs_parse_text,"typedef"))) {
    gs_parse_tok_num = gs_parse_lex_n_dump();
    res = parse_typedef(bag,definition);

  } else {
    fprintf(stderr,"Failed to parse the following symbol (not a struct neither a typedef) :\n%s\n",definition);    
    abort();
  }

  gs_parse_pointer_string_close();
  DEBUG1("end of _gs_type_parse(%s)",definition);

  return res;
}


