/**
   gen.c

   Copyright (C) 2008, RTFM, Inc.
   All Rights Reserved.

   ekr@rtfm.com  Fri May  2 22:12:12 2008
 */

//static char *RCSSTRING __UNUSED__="$Id$";

#include <string.h>
#include <stdlib.h>
#include "parser.h"


static char *camelback(char *name)
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
    
    return buf;
  }

char *name2namespace(char *name)
  {
    char *tmp=r_strdup(name);
    char *p;

    p=tmp;
    while(*p){
      if(!isalnum(*p))
        *p='_';
      p++;
    }
    
    return(tmp);
  }

static char *type2class(char *name)
  {
    char *buf;

    buf=RCALLOC(strlen(name)+10);

    sprintf(buf,"%sPdu",camelback(name));
    
    return buf;
  }


static char *s2c_decl2type(p_decl *decl)
  {
    char *buf=RMALLOC(100);

    if(decl->type==TYPE_PRIMITIVE){
      return(decl->u.primitive_.type);
    }
    else{
      snprintf(buf,100,"%s%c",type2class(decl->name),'*');
      return(buf);
    }
  }

static char *name2var(char *name)
  {
    char *buf;
    
    buf=RCALLOC(strlen(name)+10);
    
    sprintf(buf,"m%s",camelback(name));

    return buf;
  }


static char *name2enum(char *name)
  {
    char *buf;

    buf=RCALLOC(strlen(name)+10);
    
    sprintf(buf,"t%s",camelback(name));

    return buf;
  }




/* Generate H files */
int s2c_gen_hdr_h(char *name, FILE *out)
  {

    fprintf(out,"#include \"s2c_native.hxx\"\n\nnamespace s2c {\nnamespace %s {\n\n",
      name2namespace(name));
    
    return(0);
  }

static int s2c_gen_member_fxns_h(FILE *out)
  {
    /* fprintf(out,"   virtual void print(std::iostream out);\n");
      fprintf(out,"   virtual void encode(std::iostream out);\n");
      fprintf(out,"   virtual void decode(std::iostream in);\n");*/

    fprintf(out,"\n\n");
    fprintf(out,"   PDUMemberFunctions\n");
      
      return(0);
  }

static int s2c_print_a_b_indent(char *a, char *b, FILE *out)
  {
    int indent=30;

    fprintf(out, "   %s",a);
    if(strlen(a) < 30) {
      indent-=strlen(a);
    }
    else{
      indent=2;
    }

    while(indent--){
      fprintf(out," ");
    }
    
    fprintf(out, "%s;\n",b);

    return(0);
  }

static int s2c_gen_pdu_h_member(p_decl *member, FILE *out)
  {
    char buf[100];

    switch(member->type){
      case TYPE_REF:
        s2c_print_a_b_indent(s2c_decl2type(member->u.ref_.ref),
            name2var(member->name),out);

        break;
      case TYPE_VARRAY:
        snprintf(buf,sizeof(buf),"std::vector<%s>",
          s2c_decl2type(member->u.array_.ref));
        s2c_print_a_b_indent(buf,name2var(member->name),out);
        break;
      case TYPE_ARRAY:
        {
          int ct;

          if(member->u.array_.ref->type != TYPE_PRIMITIVE)
            nr_verr_exit("Don't know how to handle non-primitive arrays");
            
          if((member->u.array_.length * 8) % (member->u.array_.ref->u.primitive_.bits))
            nr_verr_exit("Non-even array length for %s",member->name);
              
          ct=(member->u.array_.length*8) / (member->u.array_.ref->u.primitive_.bits);
          
          snprintf(buf,sizeof(buf), "%s[%d]", name2var(member->name),ct);
          s2c_print_a_b_indent(s2c_decl2type(member->u.array_.ref), buf, out);

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

    s2c_gen_member_fxns_h(out);

    fprintf(out,"};\n\n");


    /* Now emit the class for each select arm */
    for(arm=STAILQ_FIRST(&decl->u.select_.arms);arm;arm=STAILQ_NEXT(arm,entry)){
      p_decl *member;

      fprintf(out,"class %s__%s : public %s {\npublic:\n", type2class(decl->name), 
        camelback(arm->name), type2class(decl->name));
      
      for(member=STAILQ_FIRST(&arm->u.select_arm_.members);member;member=STAILQ_NEXT(member,entry)){
        s2c_gen_pdu_h_member(member, out);
      }

      s2c_gen_member_fxns_h(out);

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

    s2c_gen_member_fxns_h(out);

    fprintf(out,"};\n\n\n");

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


int s2c_gen_ftr_h(FILE *out)
  {
    fprintf(out,"}\n}\n");
 
    return(0);
  }



/* Generate C files */
int s2c_gen_hdr_c(char *name,FILE *out)
  {
    fprintf(out,"#include \"%s.hxx\"\n\nnamespace s2c {\nnamespace %s {\n\n",name,
      name2namespace(name));
    
    return(0);
  }


static int s2c_gen_encode_c_simple_type(p_decl *decl, char *reference, FILE *out)
  {
    /* We can get a primitive as a ref or directly here because of arrays versus
     simple declarations*/
    if(decl->u.ref_.ref->type==TYPE_PRIMITIVE)
      fprintf(out,"   encode_uintX(out, %d, %s);\n",
        decl->u.ref_.ref->u.primitive_.bits,reference);
    else if(decl->type==TYPE_PRIMITIVE)
      fprintf(out,"   encode_uintX(out, %d, %s);\n",
        decl->u.primitive_.bits,reference);
    else
      fprintf(out,"   %s->encode(out);\n",reference);

    return(0);
  }

static int s2c_gen_encode_c_member(p_decl *member, FILE *out,int indent)
  {
    int i;

    for(i=0;i<indent;i++) fputc(' ',out);

    switch(member->type){
      case TYPE_REF:
        s2c_gen_encode_c_simple_type(member,name2var(member->name),out);
        break;
      case TYPE_VARRAY:
        {
          char reference[100];
          
          fprintf(out,"   for(int i=0;i<%s.size();i++)\n",name2var(member->name));
          snprintf(reference,sizeof(reference),"%s[i]",name2var(member->name));

          for(i=0;i<indent+3;i++) fputc(' ',out);
          s2c_gen_encode_c_simple_type(member->u.varray_.ref,reference,out);

          break;
        }
        break;
      case TYPE_ARRAY:
        {
          char reference[100];
          int ct=(member->u.array_.length*8) / (member->u.array_.ref->u.primitive_.bits);
          
          fprintf(out,"   for(int i=0;i<%d;i++)\n",ct);
          snprintf(reference,sizeof(reference),"%s[i]",name2var(member->name));

          for(i=0;i<indent+3;i++) fputc(' ',out);
          s2c_gen_encode_c_simple_type(member->u.array_.ref,reference,out);

          break;
        }
      default:
        nr_verr_exit("Don't know how to render element %s",member->name);
    }
    return(0);
    



  }

static int s2c_gen_print_c_struct(p_decl *decl, FILE *out)
  {

    fprintf(out,"void %s :: print(std::iostream *out)\n{\n",type2class(decl->name));
    fprintf(out,"   ;\n");
    fprintf(out,"};\n\n");

    return(0);
  }

static int s2c_gen_encode_c_struct(p_decl *decl, FILE *out)
  {
    p_decl *entry;

    fprintf(out,"void %s :: encode(std::iostream *out)\n{\n",type2class(decl->name));
    
    entry=STAILQ_FIRST(&decl->u.struct_.members);

    while(entry){
      s2c_gen_encode_c_member(entry, out, 0);
    
      entry=STAILQ_NEXT(entry,entry);
    }

    fprintf(out,"};\n\n");

    return(0);
  }

static int s2c_gen_decode_c_struct(p_decl *decl, FILE *out)
  {

    fprintf(out,"void %s :: decode(std::iostream *in)\n{\n",type2class(decl->name));
    fprintf(out,"   ;\n");
    fprintf(out,"};\n\n");

    return(0);
  }


static int s2c_gen_pdu_c_struct(p_decl *decl, FILE *out)
  {
    fprintf(out,"\n\n// Classes for %s */\n\n",type2class(decl->name));

    s2c_gen_print_c_struct(decl, out);
    s2c_gen_decode_c_struct(decl, out);
    s2c_gen_encode_c_struct(decl, out);
    
    return(0);
  }


static int s2c_gen_pdu_c_select(p_decl *decl, FILE *out)
  {
#if 0
    fprintf(out,"\n\n// Classes for %s */\n\n",type2class(decl->name));

    // s2c_gen_print_c_struct(decl, out);
    // s2c_gen_decode_c_struct(decl, out);
    s2c_gen_encode_c_struct(decl, out);
#endif    
    return(0);
  }

int s2c_gen_pdu_c(p_decl *decl, FILE *out)
  {
    if(decl->type == TYPE_STRUCT){
      return(s2c_gen_pdu_c_struct(decl, out));
    }
    else if(decl->type==TYPE_SELECT){
      return(s2c_gen_pdu_c_select(decl, out));
    }
    else 
      nr_verr_exit("Internal error: can't generate .c for PDU %s",decl->name);

    return(0);
  }


int s2c_gen_ftr_c(FILE *out)
  {
    fprintf(out,"}\n}\n");
 
    return(0);
  }



