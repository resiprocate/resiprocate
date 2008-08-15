/**
   ssl.y

   Copyright (C) 1998, RTFM, Inc.
   All Rights Reserved.

   ekr@rtfm.com  Fri Dec 25 20:33:47 1998
 */

/* %error-verbose */

%{

#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "r_log.h"
#include <stdio.h>

 
extern FILE *dotc;
extern FILE *doth;

#define CURRENT_DECL current_decl[current_decl_depth]

p_decl *current_decl[50]={0};
int current_decl_depth=0;
 
void push_decl(p_decl *decl)
  {
    r_log(LOG_GENERIC,LOG_DEBUG,"Pushing decl");
    current_decl[++current_decl_depth]=decl;
  }

void pop_decl()
  {
    r_log(LOG_GENERIC,LOG_DEBUG,"Popping decl %s, depth=%d",CURRENT_DECL->name,
      current_decl_depth);
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

%}
%union {
     unsigned int val;
     char str[8192];
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
%token <val> ENUM_
%token <val> DIGITALLY_SIGNED_
%token <val> COMMENT_START_
%token <str> CODE_
%token <val> COMMENT_END_
%token <val> CASE_
%token <val> CONSTANT_ 
%token <val> PRIMITIVE_
%token <val> TYPEDEF_
%token <val> OBJECT_

/*Types for nonterminals*/
%type <val> module
%type <val> typelist
%type <val> definition
 /*%type <val> selecttype */
 /* %type <val> constant_type */
 /* %type <val> selecterateds*/
 /* %type <val> selectmax*/
 /* %type <val> constval*/
%type <decl> declaration
%type <decl> select_arm
%type <decl> select
%type <str> p_type
%type <val> varray_size

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
          | enum
          | select
          | typedef
          | object
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
  STAILQ_INSERT_TAIL(&public_decls,decl,entry);  // All decls public here
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
    char *magic;

    if(r=r_assoc_fetch(types,$1, strlen($1), &v)){
      r_log(LOG_GENERIC,LOG_DEBUG,"Unknown type %s\n",$1);

      nr_verr_exit("Unknown type %s",$1);

      //v=make_fwd_ref($1);
      
    }

    decl=RCALLOC(sizeof(p_decl));
    
    
    decl->name=r_strdup($2);
    decl->type = TYPE_REF;
    decl->u.ref_.ref = v;

    if(magic=strchr(decl->name,'.')){
      magic++;
      if(!strcmp(magic,"auto_len")){
        if(decl->u.ref_.ref->type != TYPE_PRIMITIVE)
          nr_verr_exit("Auto len feature only usable with integers");
        decl->auto_len=1;
      }
      else{
        nr_verr_exit("Illegal magic operation %s",magic);
      }
    }

    $$=decl;
  };
  | NAME_ NAME_ '<' varray_size '>' ';'
  {
    int r;
    p_decl *decl;
    void *v;


    if(r=r_assoc_fetch(types,$1, strlen($1), &v)){
      r_log(LOG_GENERIC,LOG_DEBUG,"Unknown type %s\n",$1);

      nr_verr_exit("Unknown type %s",$1);
      // v=make_fwd_ref($1);
    }


    decl=RCALLOC(sizeof(p_decl));
    decl->name=r_strdup($2);
    decl->type = TYPE_VARRAY;
    decl->u.varray_.ref = v;
    decl->u.varray_.length = $4;

    $$=decl;
  };
  | NAME_ NAME_ '[' NUM_ ']' ';'
  {
    int r;
    p_decl *decl;
    void *v;


    if(r=r_assoc_fetch(types,$1, strlen($1), &v)){
      r_log(LOG_GENERIC,LOG_DEBUG,"Unknown type %s\n",$1);

      nr_verr_exit("Unknown type %s",$1);
      // v=make_fwd_ref($1);
    }


    decl=RCALLOC(sizeof(p_decl));
    decl->name=r_strdup($2);
    decl->type = TYPE_ARRAY;
    decl->u.varray_.ref = v;
    decl->u.varray_.length = $4;

    $$=decl;
  };
  | select 
  {
    $$ = $1;
  }


varray_size: 
            NUM_ DOT_DOT_ NUM_ '^' NUM_ '-' NUM_ 
            {
              unsigned long long l;
              int i;
              
              if($5 <= 0)
                nr_verr_exit("Bogus exponent %d in size expression",$5);
              l=$3;

              for(i = 1; i<$5;i++){
                l *= $3;
              }
              
              l -= $5;
              
              if(l > 0xffffffffu)
                nr_verr_exit("Overflow value in size expression");

              $$=(unsigned int)l;
            }
            | NUM_
            {
              $$=$1;
            }

p_type : NAME_
  { 
    strcpy($$,$1);
  }
  | p_type NAME_
  {
    sprintf($$,"%s %s",$1, $2);
  }
      
primitive : PRIMITIVE_ NAME_ p_type  NUM_ ';'
  {
    p_decl *decl=0;
    int r;
    decl=RCALLOC(sizeof(p_decl));

    decl->name=r_strdup($2);
    decl->u.primitive_.bits=$4;
    decl->u.primitive_.type=r_strdup($3);

    decl->type=TYPE_PRIMITIVE;

    if(r=r_assoc_insert(types,decl->name,strlen(decl->name),
         decl,0,0,R_ASSOC_NEW)){
      r_log(LOG_GENERIC,LOG_DEBUG,"Couldn't insert primitive %s. Exists?\n",decl->name);
      exit(1);
    }
  }    

object:  OBJECT_ NAME_ NAME_ ';'
  {
    p_decl *decl=0;
    int r;
    decl=RCALLOC(sizeof(p_decl));

    decl->name=r_strdup($3);
    decl->u.object_.classname=r_strdup($2);

    decl->type=TYPE_OBJECT;

    if(r=r_assoc_insert(types,decl->name,strlen(decl->name),
         decl,0,0,R_ASSOC_NEW)){
      r_log(LOG_GENERIC,LOG_DEBUG,"Couldn't insert object %s. Exists?\n",decl->name);
      exit(1);
    }
  }

enum_start: ENUM_
  {
    p_decl *decl=0;

    r_log(LOG_GENERIC,LOG_DEBUG,"enums start\n");

    decl=RCALLOC(sizeof(p_decl));

    decl->type=TYPE_ENUM;
    STAILQ_INIT(&decl->u.enum_.members);    
    push_decl(decl);
    STAILQ_INSERT_TAIL(&public_decls,decl,entry);  // All decls public here
  }

enum: enum_start '{' enumerateds '}' NAME_ ';'
{
  int r;

  CURRENT_DECL->name=r_strdup($5);
  
  r_log(LOG_GENERIC,LOG_DEBUG,"Finished with enum %s\n",$5);
  
  if(r=r_assoc_insert(types,CURRENT_DECL->name,strlen(CURRENT_DECL->name),
      CURRENT_DECL,0,0,R_ASSOC_NEW)){
    r_log(LOG_GENERIC,LOG_DEBUG,"Couldn't insert enum %s. Exists?\n",$5);
    exit(1);
  }

  pop_decl();
}

enumerateds: enumerated {};
             | enumerated ',' enumerateds {};

enumerated: NAME_ '(' NUM_ ')' 
  {
    p_decl *decl=0;
    int r;
    decl=RCALLOC(sizeof(p_decl));

    decl->name=r_strdup($1);
    decl->u.enum_value_.value=$3;
    decl->type=TYPE_ENUM_VALUE;
  
  if(r=r_assoc_insert(types,decl->name,strlen(decl->name),
      decl,0,0,R_ASSOC_NEW)){
    r_log(LOG_GENERIC,LOG_DEBUG,"Couldn't insert enum value %s. Exists?\n",$1);
    exit(1);
  }

    STAILQ_INSERT_TAIL(&CURRENT_DECL->u.enum_.members,decl,entry);
  }    
| '(' NUM_ ')'

  {
    CURRENT_DECL->u.enum_.max=$2;
  }

select: select_start '{' select_arms '}' ';' 
{
  int r;

//    CURRENT_DECL->name=r_strdup($5);
  CURRENT_DECL->name=r_strdup("auto-generated");

/*    
    r_log(LOG_GENERIC,LOG_DEBUG,"Finished with select %s\n",$5);
    
    if(r=r_assoc_insert(types,CURRENT_DECL->name,strlen(CURRENT_DECL->name),
         CURRENT_DECL,0,0,R_ASSOC_NEW)){
      r_log(LOG_GENERIC,LOG_DEBUG,"Couldn't insert struct %s. Exists?\n",$5);
      exit(1);
    }
*/
    $$ = CURRENT_DECL;

    pop_decl();
};


select_start: SELECT_ '(' NAME_ ')'
{
    p_decl *decl=0;

    r_log(LOG_GENERIC,LOG_DEBUG,"select start\n");

    decl=RCALLOC(sizeof(p_decl));

    decl->type=TYPE_SELECT;
    STAILQ_INIT(&decl->u.select_.arms);
    decl->u.select_.switch_on=r_strdup($3);
    
    push_decl(decl);
    if(!CURRENT_DECL)
      STAILQ_INSERT_TAIL(&public_decls,decl,entry);  
};

/*        | PUBLIC_ SELECT_ 
{
    p_decl *decl=0;

    r_log(LOG_GENERIC,LOG_DEBUG,"select start\n");

    decl=RCALLOC(sizeof(p_decl));

    decl->type=TYPE_SELECT;
    decl->u.select_.switch_on=r_strdup($        4);
    STAILQ_INIT(&decl->u.select_.arms);    
    push_decl(decl);
    if(!CURRENT_DECL)
      STAILQ_INSERT_TAIL(&public_decls,decl,entry);  
};
*/



select_arms : {};
             | select_arms select_arm
             {
               p_decl *decl=$2;
               
               r_log(LOG_GENERIC,LOG_DEBUG,"Adding arm %s to %s\n", decl->name, CURRENT_DECL->name);

               STAILQ_INSERT_TAIL(&CURRENT_DECL->u.select_.arms,decl,entry);
             };

select_arm_start: CASE_ NAME_ ':'
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
    if(value->type != TYPE_ENUM_VALUE)
      nr_verr_exit("%s is not a constant/enum",value->name);

    decl->type=TYPE_SELECT_ARM;
    decl->name=r_strdup($2);
    decl->u.select_arm_.value=value->u.enum_value_.value;

    STAILQ_INIT(&decl->u.select_arm_.members);
    
    push_decl(decl);
  }


select_arm: select_arm_start select_arm_decls
  {
    void *v;
    p_decl *decl=0;
    p_decl *value;
    int r;

    
    $$=CURRENT_DECL;

    pop_decl();
  }
  
select_arm_decls: {};
                | select_arm_decls declaration
  {
    p_decl *decl=$2;
    
//    r_log(LOG_GENERIC,LOG_DEBUG,"Adding type %s to %s",decl->name,CURRENT_DECL->name);
    
    STAILQ_INSERT_TAIL(&CURRENT_DECL->u.select_arm_.members,decl,entry);
  }
 | ';'
  {
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
    



