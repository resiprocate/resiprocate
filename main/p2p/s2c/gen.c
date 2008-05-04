/**
   gen.c

   Copyright (C) 2008, RTFM, Inc.
   All Rights Reserved.

   ekr@rtfm.com  Fri May  2 22:12:12 2008
 */

//static char *RCSSTRING __UNUSED__="$Id$";

#include "parser.h"

static char *type2class(char *name)
  {
    char *buf;
    int j;
    int i=0;
    int leading=1;

    buf=RCALLOC(strlen(name)+10);

    for(j=0;j<strlen(name);j++){
      if(leading){
        buf[i++]=toupper(name[j]);
        leading=0;
      }
      else{
        if(name[j]=='_'){
          leading=1;
        }
        else
          buf[i++]=name[j];
      }
    }

    strcpy(buf+i,"Pdu");
    
    return buf;
  }


static char *name2var(char *name)
  {
    char *buf;
    int j;
    int i=0;
    int leading=1;

    buf=RCALLOC(strlen(name)+10);
    
    buf[i++]='m';

    for(j=0;j<strlen(name);j++){
      if(leading){
        buf[i++]=toupper(name[j]);
        leading=0;
      }
      else{
        if(name[j]=='_'){
          leading=1;
        }
        else
          buf[i++]=name[j];
      }
    }
    buf[i]=0;

    return buf;
  }


static char *name2enum(char *name)
  {
    char *buf;
    int j;
    int i=0;
    int leading=1;

    buf=RCALLOC(strlen(name)+10);
    
    buf[i++]='t';

    for(j=0;j<strlen(name);j++){
      if(leading){
        buf[i++]=toupper(name[j]);
        leading=0;
      }
      else{
        if(name[j]=='_'){
          leading=1;
        }
        else
          buf[i++]=name[j];
      }
    }
    buf[i]=0;

    return buf;
  }


static int s2c_gen_pdu_h_member(p_decl *member, FILE *out)
  {
    switch(member->type){
      case TYPE_REF:
        fprintf(out,"   %s %s\n",type2class(member->u.ref_.ref->name),
          name2var(member->name));
        break;
      case TYPE_VARRAY:
        fprintf(out,"   vector<%s> %s\n",type2class(member->u.varray_.ref->name),
          name2var(member->name));
        break;
      case TYPE_ARRAY:
        {
          int ct;

          if(member->u.array_.ref->type != TYPE_PRIMITIVE)
            nr_verr_exit("Don't know how to handle non-primitive arrays");

            
          if((member->u.array_.length * 8) % (member->u.array_.ref->u.primitive_.bits))
            nr_verr_exit("Non-even array length for %s",member->name);
              
          ct=(member->u.array_.length*8) / (member->u.array_.ref->u.primitive_.bits);

          fprintf(out,"   %s %s[%d]\n",type2class(member->u.array_.ref->name),
            name2var(member->name),ct);
          break;
        }
      default:
        nr_verr_exit("Don't know how to render element %s",member->name);
    }
    return(0);
  }


static int s2c_gen_pdu_h_select(p_decl *decl, FILE *out)
  {
    p_decl *arm;

    /* First emit the class for the select itself */
    fprintf(out,"class %s : public PDU {\npublic:\n", type2class(decl->name));
    fprintf(out,"   int  mType;\n");
    fprintf(out,"   enum { \n");

    /* Now define the values */
    for(arm=STAILQ_FIRST(&decl->u.select_.arms);arm;arm=STAILQ_NEXT(arm,entry)){
      fprintf(out,"          %s=%d",name2enum(arm->name),arm->u.select_arm_.value);
      if(STAILQ_NEXT(arm,entry))
        fprintf(out,",\n");
      else
        fprintf(out,"\n   };\n");
    }
    fprintf(out,"};\n\n");


    /* Now emit the class for each select arm */
    for(arm=STAILQ_FIRST(&decl->u.select_.arms);arm;arm=STAILQ_NEXT(arm,entry)){
      p_decl *member;

      fprintf(out,"class %s_%s : public %s {\npublic:\n", type2class(decl->name), arm->name, type2class(decl->name));
      
      for(member=STAILQ_FIRST(&arm->u.select_arm_.members);member;member=STAILQ_NEXT(member,entry)){
        s2c_gen_pdu_h_member(member, out);
      }

      fprintf(out,"};\n\n\n");
    }

    return(0);
  }



static int s2c_gen_pdu_h_struct(p_decl *decl, FILE *out)
  {
    p_decl *entry;

    fprintf(out,"class %s : public PDU {\npublic:\n", type2class(decl->name));
    
    entry=STAILQ_FIRST(&decl->u.struct_.members);
    while(entry){
      s2c_gen_pdu_h_member(entry, out);
    
      entry=STAILQ_NEXT(entry,entry);
    }
    fprintf(out,"};\n\n\n");

    return(0);
  }

int s2c_gen_hdr_h(FILE *out)
  {
    fprintf(out,"#include \"s2c_native.hxx\"\n\nnamespace s2c {\n\n");
    
    return(0);
  }

int s2c_gen_ftr_h(FILE *out)
  {
    fprintf(out,"}\n");
    
    return(0);
  }

int s2c_gen_pdu_h(p_decl *decl, FILE *out)
  {
    if(decl->type == TYPE_STRUCT){
      return(s2c_gen_pdu_h_struct(decl, out));
    }
    else if(decl->type==TYPE_SELECT){
      return(s2c_gen_pdu_h_select(decl, out));
    }
    else 
      nr_verr_exit("Internal error: can't generate .h for PDU %s",decl->name);

    return(0);
  }

