/**
   ssl.y

   Copyright (C) 1998, RTFM, Inc.
   All Rights Reserved.

   ekr@rtfm.com  Fri Dec 25 20:33:47 1998
 */


%{

#include "parser.h"
#include "r_log.h"
#include <stdio.h>
 
extern FILE *dotc;
extern FILE *doth;

p_decl *current_decl[10]={0};
int current_decl_depth=0;
 
void push_decl(p_decl *decl)
  {
    current_decl[++current_decl_depth]=decl;
  }

void pop_decl()
  {
    current_decl_depth--;
  }

p_decl *make_fwd_ref (char *type)
  {
    p_decl *decl;

    decl=RCALLOC(sizeof(p_decl));

    decl->type=TYPE_FWDREF;
    decl->u.fwd_ref_.type=r_strdup(type);

    return(decl);
  }

#define CURRENT_DECL current_decl[current_decl_depth]
%}
%union {
     int val;
     unsigned char str[8192];
     p_decl *decl;
}


/*These tokens have attributes*/
%token <str> NAME_
%token <val> NUM_

/*Tokens*/
%token <val> DOT_DOT_
%token <val> PUBLIC_
%token <val> STRUCT_
%token <val> SELECT_
%token <str> OPAQUE_
%token <val> SELECT_
%token <val> ENUM_
%token <val> DIGITALLY_SIGNED_
%token <val> COMMENT_START_
%token <str> CODE_
%token <val> COMMENT_END_
%token <val> CASE_
%token <val> CONSTANT_ 
%token <val> PRIMITIVE_
%token <val> TYPEDEF_

/*Types for nonterminals*/
%type <val> module
%type <val> typelist
%type <val> definition
%type <val> selecttype
%type <val> constant_type
%type <val> selecterateds
%type <val> selectmax
%type <val> constval
%type <decl> declaration
%type <decl> select_arm
%type <decl> select;

/*%type <val> selecterated*/
%%
module: typelist

typelist: definition {}
| 
definition typelist {}
;

definition: 
          | primitive
          | struct_type
          | constant_type
          | enum
          | select
          | typedef
          {

          }
;


struct_start: STRUCT_ 
{
    p_decl *decl=0;

    r_log(LOG_GENERIC,LOG_DEBUG,"struct start\n");

    decl=RCALLOC(sizeof(p_decl));

    decl->type=TYPE_STRUCT;
    STAILQ_INIT(&decl->u.struct_.members);    
    push_decl(decl);
};
              | PUBLIC_ STRUCT_
{
    p_decl *decl=0;

    r_log(LOG_GENERIC,LOG_DEBUG,"struct start\n");

    decl=RCALLOC(sizeof(p_decl));

    decl->type=TYPE_STRUCT;
    STAILQ_INIT(&decl->u.struct_.members);
    push_decl(decl);

    STAILQ_INSERT_TAIL(&public_decls,decl,entry);
};


struct_type : struct_start '{' struct_decls '}' NAME_ ';'
  {
    int r;

    CURRENT_DECL->name=r_strdup($5);
    
    r_log(LOG_GENERIC,LOG_DEBUG,"Finished with struct %s\n",$5);
    
    if(r=r_assoc_insert(types,CURRENT_DECL->name,strlen(CURRENT_DECL->name),
         CURRENT_DECL,0,0,R_ASSOC_NEW)){
      r_log(LOG_GENERIC,LOG_DEBUG,"Couldn't insert struct %s. Exists?\n",$5);
      exit(1);
    }
    pop_decl();
  }

struct_decls : {};
             | struct_decls declaration  
             {
               p_decl *decl=$2;
               
               r_log(LOG_GENERIC,LOG_DEBUG,"Adding type %s to %s\n", decl->name, CURRENT_DECL->name);

               STAILQ_INSERT_TAIL(&CURRENT_DECL->u.struct_.members,decl,entry);
             };


declaration : NAME_ NAME_ ';'
  { 
    p_decl *decl=0;
    void *v;
    int r;


    if(r=r_assoc_fetch(types,$1, strlen($1), &v)){
      r_log(LOG_GENERIC,LOG_DEBUG,"Unknown type %s\n",$1);

      v=make_fwd_ref($1);

    }
    decl=RCALLOC(sizeof(p_decl));

    decl->name=r_strdup($2);
    decl->type = TYPE_REF;
    decl->u.ref_.ref = v;

    $$=decl;
  };
  | NAME_ NAME_ '<' NUM_ '>' ';'
  {
    int r;
    p_decl *decl;
    void *v;

    if(r=r_assoc_fetch(types,$1, strlen($1), &v)){
      r_log(LOG_GENERIC,LOG_DEBUG,"Unknown type %s\n",$1);

      v=make_fwd_ref($1);
    }

    decl=RCALLOC(sizeof(p_decl));
    decl->name=r_strdup($2);
    decl->type = TYPE_VARRAY;
    decl->u.varray_.ref = v;
    decl->u.varray_.length = -1 * $4;

    $$=decl;
  };

primitive : PRIMITIVE_ NAME_ NUM_ ';'
  {
    p_decl *decl=0;
    int r;
    decl=RCALLOC(sizeof(p_decl));

    decl->name=r_strdup($2);
    decl->u.primitive_.bits=$3;
    decl->type=TYPE_PRIMITIVE;

    if(r=r_assoc_insert(types,decl->name,strlen(decl->name),
         decl,0,0,R_ASSOC_NEW)){
      r_log(LOG_GENERIC,LOG_DEBUG,"Couldn't insert primitive %s. Exists?\n",decl->name);
      exit(1);
    }
  }    

enum: ENUM_ '{' enumerateds '}' NAME_ ';'
  
enumerateds: enumerated {};
             | enumerated ',' enumerateds {};

enumerated: NAME_ '(' NUM_ ')' 
  {
    p_decl *decl=0;
    int r;
    decl=RCALLOC(sizeof(p_decl));

    decl->name=r_strdup($1);
    decl->u.enum_.value=$3;
    decl->type=TYPE_ENUM;

    if(r=r_assoc_insert(types,decl->name,strlen(decl->name),
         decl,0,0,R_ASSOC_NEW)){
      r_log(LOG_GENERIC,LOG_DEBUG,"Couldn't insert enum %s. Exists?\n",decl->name);
      exit(1);
    }
  }    


select: select_start '{' select_arms '}' NAME_ ';' 
{
  int r;

    CURRENT_DECL->name=r_strdup($5);
    
    r_log(LOG_GENERIC,LOG_DEBUG,"Finished with select %s\n",$5);
    
    if(r=r_assoc_insert(types,CURRENT_DECL->name,strlen(CURRENT_DECL->name),
         CURRENT_DECL,0,0,R_ASSOC_NEW)){
      r_log(LOG_GENERIC,LOG_DEBUG,"Couldn't insert struct %s. Exists?\n",$5);
      exit(1);
    }
    $$ = CURRENT_DECL;

    pop_decl();
};

select_start: SELECT_ 
{
    p_decl *decl=0;

    r_log(LOG_GENERIC,LOG_DEBUG,"select start\n");

    decl=RCALLOC(sizeof(p_decl));

    decl->type=TYPE_SELECT;
    STAILQ_INIT(&decl->u.select_.arms);    
    push_decl(decl);
};
        | PUBLIC_ SELECT_ 
{
    p_decl *decl=0;

    r_log(LOG_GENERIC,LOG_DEBUG,"select start\n");

    decl=RCALLOC(sizeof(p_decl));

    decl->type=TYPE_SELECT;
    STAILQ_INIT(&decl->u.select_.arms);    
    push_decl(decl);
    STAILQ_INSERT_TAIL(&public_decls,decl,entry);
};



select_arms : {};
             | select_arms select_arm
             {
               p_decl *decl=$2;
               
               r_log(LOG_GENERIC,LOG_DEBUG,"Adding arm %s to %s\n", decl->name, CURRENT_DECL->name);

               STAILQ_INSERT_TAIL(&CURRENT_DECL->u.select_.arms,decl,entry);
             };

select_arm: CASE_ NAME_ ':' declaration
  {
    void *v;
    p_decl *decl=0;
    p_decl *value;
    int r;
    decl=RCALLOC(sizeof(p_decl));

    if(r=r_assoc_fetch(types,$2, strlen($2), &v)){
      nr_verr_exit("Unknown value %s\n",$2);
      exit(1);
    }
    value=v;
    if(value->type != TYPE_ENUM)
      nr_verr_exit("%s is not a constant/enum",value->name);

    decl->name=r_strdup($2);
    decl->u.select_arm_.ref=$4;
    decl->u.select_arm_.value=value->u.enum_.value;

    decl->type=TYPE_SELECT_ARM;

    $$=decl;
  }
           

typedef: TYPEDEF_ declaration
  {
    int r;

    if(r=r_assoc_insert(types,$2->name,strlen($2->name),
         $2,0,0,R_ASSOC_NEW)){
      r_log(LOG_GENERIC,LOG_DEBUG,"Couldn't insert struct %s. Exists?\n",$2->name);
      exit(1);
    }
  };
    



