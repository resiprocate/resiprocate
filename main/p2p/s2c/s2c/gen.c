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

    sprintf(buf,"%sStruct",camelback(name));
    
    return buf;
  }


static char *s2c_decl2type(p_decl *decl)
  {
    char *buf=RMALLOC(100);

    if(decl->type==TYPE_PRIMITIVE){
      return(decl->u.primitive_.type);
    }
    else if(decl->type==TYPE_ENUM){
      return(camelback(decl->name));
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

static int max2bytes(UINT4 max)
  {
    int b=0;

    while(max){
      b++;
      max >>=8;
    }
    
    return b;
  }



/* Generate H files */
int s2c_gen_hdr_h(char *name, FILE *out)
  {

    fprintf(out,"#include \"p2p/s2c/s2c/s2c_native.hxx\"\n\nnamespace s2c {\n\n",
      name2namespace(name));
    
    return(0);
  }

static int s2c_gen_member_fxns_h(char *classname, char *name, FILE *out)
  {
    fprintf(out,"\n\n");
    fprintf(out,"   %s() {mName = \"%s\";}\n", classname, name);
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
          s2c_decl2type(member->u.varray_.ref));
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


    fprintf(out, "\n   struct {\n");
    
    /* Now emit the union for each select arm */
    for(arm=STAILQ_FIRST(&decl->u.select_.arms);arm;arm=STAILQ_NEXT(arm,entry)){
      p_decl *member;
      char armname[100];
      
      snprintf(armname,100,"m%s",camelback(arm->name));

      fprintf(out,"     struct {\n");

      for(member=STAILQ_FIRST(&arm->u.select_arm_.members);member;member=STAILQ_NEXT(member,entry)){
              fprintf(out,"       ");
              s2c_gen_pdu_h_member(member, out);
      }
      
      fprintf(out,"     } %s;\n",armname);
    }

    fprintf(out, "   } u;\n");

    s2c_gen_member_fxns_h(type2class(decl->name),decl->name,out);

    fprintf(out,"};\n\n");



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

    s2c_gen_member_fxns_h(type2class(decl->name),decl->name, out);

    fprintf(out,"};\n\n\n");

    return(0);
  }

static int s2c_gen_pdu_h_enum(p_decl *decl, FILE *out)
  {
    p_decl *entry;

    fprintf(out,"enum %s{\n", camelback(decl->name));
    
    entry=STAILQ_FIRST(&decl->u.enum_.members);
    while(entry){
      fprintf(out,"   %s = %d",entry->name,entry->u.enum_value_.value);
      entry=STAILQ_NEXT(entry,entry);
      if(entry){
        fprintf(out,",\n");
      }
      else{
        fprintf(out,"\n");
      }
    }
    
    fprintf(out,"};\n\n");
    
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
    else if(decl->type==TYPE_ENUM){
      return(s2c_gen_pdu_h_enum(decl,out));
    }
    else 
      nr_verr_exit("Internal error: can't generate .h for PDU %s",decl->name);

    return(0);
  }


int s2c_gen_ftr_h(FILE *out)
  {
    fprintf(out,"}\n");
 
    return(0);
  }



/* Generate C files */
int s2c_gen_hdr_c(char *name,FILE *out)
  {
    fprintf(out,"#include <iostream>\n#include <iomanip>\n#include \"rutil/Logger.hxx\"\n#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST\n#include \"%sGen.hxx\"\n\nnamespace s2c {\n\n\n",name,
      name2namespace(name));
    
    return(0);
  }


static int s2c_gen_encode_c_simple_type(p_decl *decl, char *reference, FILE *out)
  {
    /* We can get a primitive as a ref or directly here because of arrays versus
     simple declarations*/
    if(decl->u.ref_.ref->type==TYPE_PRIMITIVE){
      fprintf(out,"   encode_uintX(out, %d, %s);\n",
        decl->u.ref_.ref->u.primitive_.bits,reference);
    }
    else if(decl->u.ref_.ref->type==TYPE_ENUM_VALUE){
      fprintf(stderr,"XXX\n");
    }
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
          int lengthbytes=max2bytes(member->u.varray_.length);

          fprintf(out,"   long pos1=out.tellp();\n");
          fprintf(out,"   out.seekp(pos1 + %d);\n",lengthbytes);
          
          fprintf(out,"   for(unsigned int i=0;i<%s.size();i++)\n",name2var(member->name));
          snprintf(reference,sizeof(reference),"%s[i]",name2var(member->name));

          for(i=0;i<indent+3;i++) fputc(' ',out);
          s2c_gen_encode_c_simple_type(member->u.varray_.ref,reference,out);

          fprintf(out,"   long pos2=out.tellp();\n");
          fprintf(out,"   out.seekp(pos1);\n");
          fprintf(out,"   encode_uintX(out, %d, (pos2 - pos1) - %d);\n",
            lengthbytes*8, lengthbytes);
          fprintf(out,"   out.seekp(pos2);\n");

          break;
        }
        break;
      case TYPE_ARRAY:
        {
          char reference[100];
          int ct=(member->u.array_.length*8) / (member->u.array_.ref->u.primitive_.bits);
          
          fprintf(out,"   for(unsigned int i=0;i<%d;i++)\n",ct);
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


static int s2c_gen_encode_c_struct(p_decl *decl, FILE *out)
  {
    p_decl *entry;

    fprintf(out,"void %s :: encode(std::ostream& out)\n{\n",type2class(decl->name));
    
    fprintf(out,"   DebugLog(<< \"Encoding %s\");\n",type2class(decl->name));
    entry=STAILQ_FIRST(&decl->u.struct_.members);

    while(entry){
      s2c_gen_encode_c_member(entry, out, 0);
      
      fprintf(out,"\n");
      entry=STAILQ_NEXT(entry,entry);
    }

    fprintf(out,"};\n\n");

    return(0);
  }

static int s2c_gen_encode_c_select(p_decl *decl, FILE *out)
  {
#if 0
    p_decl *arm,*entry;

    fprintf(out,"void %s :: encode(std::ostream& out)\n{\n",type2class(decl->name));
    
    fprintf(out,"   DebugLog(<< \"Encoding %s\");\n",type2class(decl->name));

    fprintf(out,"   switch(mType) {\n");
    arm=STAILQ_FIRST(&decl->u.select_.arms);
    while(arm){
      fprintf(out,"      case %d:\n",arm->u.select_arm_.value);
      entry=STAILQ_FIRST(&arm->u.select_arm_.members);
      while(entry){
        s2c_gen_encode_c_member(entry, out, 9);
        entry=STAILQ_NEXT(entry,entry);
      }
      fprintf(out,"          break;\n\n");
      arm=STAILQ_NEXT(arm,entry);
      fprintf(out,"       default: /* User error */ \n");
      fprintf(out,"          assert(1==0);\n");
      fprintf(out,"   }\n");
    }

    fprintf(out,"};\n\n");
#endif

    return(0);
  }


static int s2c_gen_print_c_simple_type(p_decl *decl, char *reference, FILE *out)
  {
    /* We can get a primitive as a ref or directly here because of arrays versus
     simple declarations*/
    
    if(decl->u.ref_.ref->type==TYPE_PRIMITIVE){
      fprintf(out,"   do_indent(out, indent);\n");
      fprintf(out,"   (out)  << \"%s:\" << std::hex << (unsigned long long)%s << \"\\n\"; \n", decl->name, reference);
    }
    else if(decl->type==TYPE_PRIMITIVE){
      fprintf(out,"   do_indent(out, indent);\n");
      fprintf(out,"   (out)  << \"%s:\" << std::hex << (unsigned long long) %s << \"\\n\"; \n", decl->name, reference);
    }
    else
      fprintf(out,"   %s->print(out, indent);\n",reference);

    return(0);
  }


static int s2c_gen_print_c_member(p_decl *member, FILE *out)
  {
    int i;
    char reference[20];

    switch(member->type){
      case TYPE_REF:
        s2c_gen_print_c_simple_type(member,name2var(member->name),out);
        break;
      case TYPE_VARRAY:
        {
          fprintf(out,"   for(unsigned int i=0;i<%s.size();i++){\n",name2var(member->name));
          snprintf(reference,sizeof(reference),"%s[i]",name2var(member->name));

          for(i=0;i<3;i++) fputc(' ',out);
          s2c_gen_print_c_simple_type(member->u.varray_.ref,reference,out);
          fprintf(out,"   }\n");
          break;
        }
        break;
      case TYPE_ARRAY:
        {
          char reference[100];
          int ct=(member->u.array_.length*8) / (member->u.array_.ref->u.primitive_.bits);
          
          fprintf(out,"   for(unsigned int i=0;i<%d;i++) {\n",ct);
          snprintf(reference,sizeof(reference),"%s[i]",name2var(member->name));
          for(i=0;i<3;i++) fputc(' ',out);
          s2c_gen_print_c_simple_type(member->u.array_.ref,reference,out);
          fprintf(out,"   }\n");

          break;
        }
      default:
        nr_verr_exit("Don't know how to render element %s",member->name);
    }
    return(0);
  }



static int s2c_gen_print_c_struct(p_decl *decl, FILE *out)
  {
    p_decl *entry;

    fprintf(out,"void %s :: print(std::ostream& out, int indent) const\n{\n",type2class(decl->name));
    fprintf(out,"   do_indent(out,indent);\n");
    fprintf(out,"   (out) << \"%s:\\n\";\n",decl->name);
    fprintf(out,"   indent+=2;\n");

    entry=STAILQ_FIRST(&decl->u.struct_.members);

    while(entry){
      s2c_gen_print_c_member(entry, out);

      entry=STAILQ_NEXT(entry,entry);
    }

    fprintf(out,"};\n\n");

    return(0);
  }

static int s2c_gen_decode_c_simple_type(p_decl *decl, char *reference, char *instream, FILE *out)
  {
    /* We can get a primitive as a ref or directly here because of arrays versus
     simple declarations*/
    if(decl->u.ref_.ref->type==TYPE_PRIMITIVE){
      fprintf(out,"   decode_uintX(%s, %d, %s);\n",
        instream,decl->u.ref_.ref->u.primitive_.bits,reference);
      fprintf(out," DebugLog( << \"%s\");\n",reference);
    }
    else if(decl->type==TYPE_PRIMITIVE){
      fprintf(out,"   decode_uintX(%s, %d, %s);\n",
        instream,decl->u.primitive_.bits,reference);
      fprintf(out," DebugLog( << \"%s\");\n",reference);
    }
    else{
      fprintf(out,"   %s = new %s();\n",reference,type2class(decl->u.ref_.ref->name));
      fprintf(out,"   %s->decode(%s);\n",reference,instream);
    }
    return(0);
  }

static int s2c_gen_decode_c_member(p_decl *member, FILE *out,int indent)
  {
    int i;

    for(i=0;i<indent;i++) fputc(' ',out);

    switch(member->type){
      case TYPE_REF:
        s2c_gen_decode_c_simple_type(member,name2var(member->name),"in",out);
        break;
      case TYPE_VARRAY:
        {
          char reference[100];
          
          fprintf(out,"   resip::Data d;\n");
          fprintf(out,"   read_varray1(in, %d, d);\n",max2bytes(member->u.varray_.length));
          fprintf(out,"   resip::DataStream in2(d);\n");
          fprintf(out,"   int i=0;\n");
          fprintf(out,"   while(in2.peek()!=EOF){\n");
          fprintf(out,"      %s.push_back(0);\n",name2var(member->name));
          snprintf(reference,sizeof(reference),"%s[i++]",name2var(member->name));
          for(i=0;i<indent+3;i++) fputc(' ',out);
          s2c_gen_decode_c_simple_type(member->u.varray_.ref,reference,"in2",out);
          fprintf(out,"   }\n;");
        }
        break;
      case TYPE_ARRAY:
        {
          char reference[100];
          int ct=(member->u.array_.length*8) / (member->u.array_.ref->u.primitive_.bits);
          
          fprintf(out,"   for(unsigned int i=0;i<%d;i++)\n",ct);
          snprintf(reference,sizeof(reference),"%s[i]",name2var(member->name));

          for(i=0;i<indent+3;i++) fputc(' ',out);
          s2c_gen_decode_c_simple_type(member->u.array_.ref,reference,"in",out);

          break;
        }
      default:
        nr_verr_exit("Don't know how to render element %s",member->name);
    }
    return(0);
  }

static int s2c_gen_decode_c_struct(p_decl *decl, FILE *out)
  {
    p_decl *entry;

    fprintf(out,"void %s :: decode(std::istream& in)\n{\n",type2class(decl->name));
    
    fprintf(out," DebugLog(<< \"Decoding %s\");\n",type2class(decl->name));
    entry=STAILQ_FIRST(&decl->u.struct_.members);

    while(entry){
      s2c_gen_decode_c_member(entry, out, 0);
      
      fprintf(out,"\n");
      entry=STAILQ_NEXT(entry,entry);
    }

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
    fprintf(out,"\n\n// Classes for %s */\n\n",type2class(decl->name));

    // s2c_gen_print_c_struct(decl, out);
    // s2c_gen_decode_c_struct(decl, out);
    s2c_gen_encode_c_select(decl, out);

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
    else if(decl->type==TYPE_ENUM){
      // Do nothing
    }
    else 
      nr_verr_exit("Internal error: can't generate .c for PDU %s",decl->name);

    return(0);
  }


int s2c_gen_ftr_c(FILE *out)
  {
    fprintf(out,"}\n");
 
    return(0);
  }



