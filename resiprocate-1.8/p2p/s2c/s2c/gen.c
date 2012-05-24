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


static int s2c_gen_pdu_h_select(p_decl *decl, FILE *out, int do_inline);
static int s2c_gen_encode_c_select(p_decl *decl, FILE *out, int do_inline);
static int s2c_gen_decode_c_select(p_decl *decl, FILE *out,char *instream,int do_inline);
static int s2c_gen_print_c_select(p_decl *decl, FILE *out,int do_inline);
static int s2c_gen_construct_c_member(p_decl *member, FILE *out,int indent, char *prefix);

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
    if(decl->type==TYPE_OBJECT){
      return(decl->u.object_.classname);
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

static int is_opaque(p_decl *decl)
  {
    if(!strcmp(decl->name,"opaque"))
      return 1;
    return 0;
  }

/* Generate H files */
int s2c_gen_hdr_h(char *name, FILE *out)
  {

    fprintf(out,"#ifndef _%s_h_\n#define _%s_h_\n\n",name,name);
    fprintf(out,"#include \"p2p/s2c/s2c/s2c_native.hxx\"\n\nnamespace s2c {\n\n",
      name2namespace(name));
    
    return(0);
  }

static int s2c_gen_member_fxns_h(char *classname, char *name, FILE *out)
  {
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
        if(!is_opaque(member->u.varray_.ref)){
          snprintf(buf,sizeof(buf),"std::vector<%s>",
            s2c_decl2type(member->u.varray_.ref));
          s2c_print_a_b_indent(buf,name2var(member->name),out);
        }
        else{
          s2c_print_a_b_indent("resip::Data",name2var(member->name),out);
        }
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
      case TYPE_SELECT:
        {
          s2c_gen_pdu_h_select(member, out, 1);
        }
        break;
      default:
        nr_verr_exit("Don't know how to render element %s",member->name);
    }
    return(0);
  }


static int s2c_gen_pdu_h_select(p_decl *decl, FILE *out, int do_inline)
  {
    p_decl *arm;

    if(!do_inline){
      /* First emit the class for the select itself */
      fprintf(out,"class %s : public PDU {\npublic:\n", type2class(decl->name));
      fprintf(out,"   %s();\n",type2class(decl->name));
    }

    /* Now emit each select arm */
    for(arm=STAILQ_FIRST(&decl->u.select_.arms);arm;arm=STAILQ_NEXT(arm,entry)){
      p_decl *member;
      char armname[100];
      
      snprintf(armname,100,"m%s",camelback(arm->name));

      fprintf(out,"   struct %s_ {\n", armname);

      for(member=STAILQ_FIRST(&arm->u.select_arm_.members);member;member=STAILQ_NEXT(member,entry)){
              fprintf(out,"     ");
              s2c_gen_pdu_h_member(member, out);
      }
      
      fprintf(out,"   } %s;\n",armname);
    }

    if(!do_inline){
      s2c_gen_member_fxns_h(type2class(decl->name),decl->name,out);
      
      fprintf(out,"};\n\n");
    }

    return(0);
  }



static int s2c_gen_pdu_h_struct(p_decl *decl, FILE *out)
  {
    p_decl *entry;

    fprintf(out,"class %s : public PDU {\npublic:\n", type2class(decl->name));
    fprintf(out,"   %s();\n",type2class(decl->name));    
    fprintf(out,"   %s(const %s&);\n",type2class(decl->name),type2class(decl->name));

    entry=STAILQ_FIRST(&decl->u.struct_.members);
    while(entry){
      if(!entry->auto_len)
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

    fprintf(out,"typedef enum {\n");
    
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
    
    fprintf(out,"} %s;\n\n", camelback(decl->name));
    
    return(0);
  }

int s2c_gen_pdu_h(p_decl *decl, FILE *out)
  {
    if(decl->type == TYPE_STRUCT){
      return(s2c_gen_pdu_h_struct(decl, out));
    }
    else if(decl->type==TYPE_SELECT){
      return(s2c_gen_pdu_h_select(decl, out,0));
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
    fprintf(out,"}\n\n#endif\n");
 
    return(0);
  }



/* Generate C files */
int s2c_gen_hdr_c(char *name,FILE *out)
  {
    fprintf(out,"#include <iostream>\n");
    fprintf(out,"#include <iomanip>\n");
    fprintf(out,"#include \"rutil/Logger.hxx\"\n");
    fprintf(out,"#include \"rutil/ParseException.hxx\"\n");
    fprintf(out,"#include \"rutil/Data.hxx\"\n");
    fprintf(out,"#include \"rutil/DataStream.hxx\"\n");
    fprintf(out,"#include \"P2PSubsystem.hxx\"\n");
    fprintf(out,"#define RESIPROCATE_SUBSYSTEM P2PSubsystem::P2P\n");
    fprintf(out,"#include \"%sGen.hxx\"\n",name);
    fprintf(out,"#include <assert.h>\n");
    fprintf(out,"\n");
    fprintf(out,"namespace s2c {\n");
    fprintf(out,"\n");fprintf(out,"\n");
    
    fprintf(out,"/* EKR: Most unprincipled hack of the day */\n");
    fprintf(out,"#define TELLP(a) (static_cast<resip::DataStream &>(a)).tellp()\n");
    fprintf(out,"#define SEEKP(a,b) (static_cast<resip::DataStream &>(a)).seekp(b)\n");

    return(0);
  }


static int s2c_gen_encode_c_simple_type(p_decl *decl, char *prefix, char *reference, FILE *out)
  {
    /* We can get a primitive as a ref or directly here because of arrays versus
     simple declarations*/
    if(decl->u.ref_.ref->type==TYPE_PRIMITIVE){
      fprintf(out,"   StackLog( << \"%s%s =\" << std::hex << (unsigned long long) %s%s );\n",prefix,reference,
        prefix,reference);
      fprintf(out,"   encode_uintX(out, %d, %s%s);\n",
        decl->u.ref_.ref->u.primitive_.bits,prefix,reference);
    }
    else if(decl->u.ref_.ref->type==TYPE_ENUM){
      fprintf(out,"   encode_uintX(out, %d, (u_int64)(%s%s));\n",
        8*max2bytes(decl->u.ref_.ref->u.enum_.max),prefix,reference);
    }
    else if(decl->type==TYPE_PRIMITIVE){
      fprintf(out,"   StackLog( << \"%s%s =\" << std::hex << (unsigned long long) %s%s );\n",prefix,reference,
        prefix,reference);
      fprintf(out,"   encode_uintX(out, %d, %s%s);\n",
        decl->u.primitive_.bits,prefix,reference);
    }
    else if(decl->type==TYPE_OBJECT){
      fprintf(out,"   out << %s%s;\n",prefix,reference);
    }
    else{
      fprintf(out,"   %s%s->encode(out);\n",prefix,reference);
    }
    return(0);
  }


static int s2c_gen_encode_c_member(p_decl *member, FILE *out,int indent, char *prefix)
  {
    int i;

    for(i=0;i<indent;i++) fputc(' ',out);

    switch(member->type){
      case TYPE_REF:
        s2c_gen_encode_c_simple_type(member,prefix,name2var(member->name),out);
        break;
      case TYPE_VARRAY:
        {
          char reference[100];
          int lengthbytes=max2bytes(member->u.varray_.length);
          
          if(!is_opaque(member->u.varray_.ref)){
              fprintf(out,"   {\n");
              fprintf(out,"   long pos1=TELLP(out);\n");
              fprintf(out,"   for(int i=0;i<%d;i++) out.put(0);\n",lengthbytes);
          
              fprintf(out,"   for(unsigned int i=0;i<%s%s.size();i++)\n",prefix,name2var(member->name));
              fprintf(out,"   {\n");
              snprintf(reference,sizeof(reference),"%s%s[i]",prefix,name2var(member->name));

              for(i=0;i<indent+3;i++) fputc(' ',out);
              s2c_gen_encode_c_simple_type(member->u.varray_.ref,"",reference,out);
              fprintf(out,"   }\n");
              fprintf(out,"   long pos2=TELLP(out);\n");
              fprintf(out,"   SEEKP(out,pos1);\n");
              fprintf(out,"   encode_uintX(out, %d, (pos2 - pos1) - %d);\n",
                lengthbytes*8, lengthbytes);
              fprintf(out,"   SEEKP(out,pos2);\n");
              fprintf(out,"   }\n");
              break;
            }
            else {
              // Special case Data for Duane
              fprintf(out,"    encode_uintX(out, %d, %s%s.size());\n",
                8*lengthbytes, prefix, name2var(member->name));
              fprintf(out,"    out << %s%s;\n",prefix,name2var(member->name));
            }
        }
        break;
      case TYPE_ARRAY:
        {
          char reference[100];
          int ct=(member->u.array_.length*8) / (member->u.array_.ref->u.primitive_.bits);
          
          fprintf(out,"   for(unsigned int i=0;i<%d;i++){\n",ct);
          snprintf(reference,sizeof(reference),"%s%s[i]",prefix,name2var(member->name));

          for(i=0;i<indent+3;i++) fputc(' ',out);
          s2c_gen_encode_c_simple_type(member->u.array_.ref,"",reference,out);

          fprintf(out,"   }\n");
          break;
        }
      case TYPE_SELECT:
        {
          s2c_gen_encode_c_select(member, out, 1);
        }
        break;
      default:
        nr_verr_exit("Don't know how to render element %s",member->name);
    }
    return(0);
  }


static int s2c_gen_encode_c_struct(p_decl *decl, FILE *out)
  {
    p_decl *entry;
    int do_auto=0;

    fprintf(out,"void %s :: encode(std::ostream& out) const \n{\n",type2class(decl->name));
    
    fprintf(out,"   StackLog(<< \"Encoding %s\");\n",type2class(decl->name));
    entry=STAILQ_FIRST(&decl->u.struct_.members);

    while(entry){
      if(entry->auto_len){
        if(do_auto)
          nr_verr_exit("Auto len feature can only be used once per struct");
        
        do_auto=entry->u.ref_.ref->u.primitive_.bits;

        fprintf(out,"   long pos1=TELLP(out);\n");
        fprintf(out,"   for(int i=0;i<%d;i++) out.put(0);\n",do_auto/8);
      }
      else{
        s2c_gen_encode_c_member(entry, out, 0,"");
      }

      fprintf(out,"\n");
      entry=STAILQ_NEXT(entry,entry);
    }

    if(do_auto){
      fprintf(out,"   long pos2=TELLP(out);\n");
      fprintf(out,"   SEEKP(out,pos1);\n");
      fprintf(out,"   encode_uintX(out, %d, (pos2 - pos1) - %d);\n",
        do_auto, do_auto/8);
      fprintf(out,"   SEEKP(out,pos2);\n");
    }

    fprintf(out,"};\n\n");

    return(0);
  }

static int s2c_gen_encode_c_select(p_decl *decl, FILE *out, int do_inline)
  {
    p_decl *arm,*entry;
    char prefix[100];

    if(!do_inline){
      fprintf(out,"void %s :: encode(std::ostream& out) const\n{\n",type2class(decl->name));
      
      fprintf(out,"   StackLog(<< \"Encoding %s\");\n",type2class(decl->name));
    }

    fprintf(out,"   switch(%s) {\n",name2var(decl->u.select_.switch_on));
    arm=STAILQ_FIRST(&decl->u.select_.arms);
    while(arm){
      fprintf(out,"      case %d:\n",arm->u.select_arm_.value);
      entry=STAILQ_FIRST(&arm->u.select_arm_.members);
      while(entry){
        snprintf(prefix,100,"m%s.",camelback(arm->name));
        s2c_gen_encode_c_member(entry, out, 9, prefix);
        entry=STAILQ_NEXT(entry,entry);
      }
      fprintf(out,"          break;\n\n");
      arm=STAILQ_NEXT(arm,entry);
    }
    fprintf(out,"       default: /* User error */ \n");
    fprintf(out,"          assert(1==0);\n");
    fprintf(out,"   }\n\n");
    
    if(!do_inline){
      fprintf(out,"};\n");
    }
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
    else if(decl->u.ref_.ref->type==TYPE_ENUM){
      fprintf(out,"   do_indent(out, indent);\n");
      fprintf(out,"   (out)  << \"%s:\" << std::hex << (unsigned long long) %s << \"\\n\"; \n", decl->name, reference);
    }
    
    else if(decl->type==TYPE_PRIMITIVE){
      fprintf(out,"   do_indent(out, indent);\n");
      fprintf(out,"   (out)  << \"%s:\" << std::hex << (unsigned long long) %s << \"\\n\"; \n", decl->name, reference);
    }
    else if(decl->type==TYPE_OBJECT){
      ; /* TODO */
    }
    else
      fprintf(out,"   %s->print(out, indent);\n",reference);

    return(0);
  }


static int s2c_gen_print_c_member(p_decl *member, FILE *out)
  {
    int i;
    char reference[100];

    switch(member->type){
      case TYPE_REF:
        s2c_gen_print_c_simple_type(member,name2var(member->name),out);
        break;
      case TYPE_VARRAY:
        if(!is_opaque(member->u.varray_.ref)){
          fprintf(out,"   for(unsigned int i=0;i<%s.size();i++){\n",name2var(member->name));
          snprintf(reference,sizeof(reference),"%s[i]",name2var(member->name));
          
          for(i=0;i<3;i++) fputc(' ',out);
          s2c_gen_print_c_simple_type(member->u.varray_.ref,reference,out);
          fprintf(out,"   }\n");
        }
        else {
          /* Special case Data for duane */
          fprintf(out, "    out << %s.hex();\n",name2var(member->name));
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
      case TYPE_SELECT:
        {
          s2c_gen_print_c_select(member,out,1);
        }
        break;
      default:
        nr_verr_exit("Don't know how to render element %s",member->name);
    }
    return(0);
  }



static int s2c_gen_print_c_struct(p_decl *decl, FILE *out)
  {
    p_decl *entry;

    fprintf(out,"void %s :: print(std::ostream& out, int indent) const \n{\n",type2class(decl->name));
    fprintf(out,"   do_indent(out,indent);\n");
    fprintf(out,"   (out) << \"%s:\\n\";\n",decl->name);
    fprintf(out,"   indent+=2;\n");

    entry=STAILQ_FIRST(&decl->u.struct_.members);

    while(entry){
      if(!entry->auto_len)
        s2c_gen_print_c_member(entry, out);

      entry=STAILQ_NEXT(entry,entry);
    }

    fprintf(out,"};\n\n");

    return(0);
  }

static int s2c_gen_decode_c_simple_type(p_decl *decl, char *prefix, char *reference, char *instream, FILE *out)
  {
    /* We can get a primitive as a ref or directly here because of arrays versus
     simple declarations*/
    if(decl->u.ref_.ref->type==TYPE_PRIMITIVE){
      fprintf(out,"   decode_uintX(%s, %d, %s%s);\n",
        instream,decl->u.ref_.ref->u.primitive_.bits,prefix,reference);
      fprintf(out,"   StackLog( << \"%s%s =\" << std::hex << (unsigned long long) %s%s );\n",prefix,reference,
        prefix,reference);
    }
    else if(decl->u.ref_.ref->type==TYPE_ENUM){
      fprintf(out,"   {\n");
      fprintf(out,"      u_int32 v;\n");
      fprintf(out,"      decode_uintX(%s, %d, v);\n",instream,
        8*max2bytes(decl->u.ref_.ref->u.enum_.max));
      fprintf(out,"      %s%s=(%s)v;\n",prefix,reference,decl->u.ref_.ref->name);
      fprintf(out,"   }\n");
      
    }
    else if(decl->type==TYPE_PRIMITIVE){
      fprintf(out,"   decode_uintX(%s, %d, %s%s);\n",
        instream,decl->u.primitive_.bits,prefix,reference);
      fprintf(out,"   StackLog( << \"%s%s =\" << std::hex << (unsigned long long) %s%s );\n",prefix,reference,
        prefix,reference);
    }
    else if(decl->type==TYPE_OBJECT){
      fprintf(out,"   %s%s << %s;\n",prefix,reference,instream);
    }
    else{
      if(decl->type==TYPE_REF){
        fprintf(out,"   %s%s = new %s();\n",prefix,reference,type2class(decl->u.ref_.ref->name));
      }
      else{
        fprintf(out,"   %s%s = new %s();\n",prefix,reference,type2class(decl->name));
      }

      fprintf(out,"   %s%s->decode(%s);\n",prefix,reference,instream);
    }
    return(0);
  }

static int s2c_gen_decode_c_member(p_decl *member, FILE *out,char *instream,int indent, char *prefix)
  {
    int i;

    for(i=0;i<indent;i++) fputc(' ',out);

    switch(member->type){
      case TYPE_REF:
        s2c_gen_decode_c_simple_type(member,prefix,name2var(member->name),instream,out);
        break;
      case TYPE_VARRAY:
        if(!is_opaque(member->u.varray_.ref)){
          char reference[100];
          
          fprintf(out,"   {\n");
          fprintf(out,"   resip::Data d;\n");
          fprintf(out,"   read_varray1(in, %d, d);\n",max2bytes(member->u.varray_.length));
          fprintf(out,"   resip::DataStream in2(d);\n");
          fprintf(out,"   int i=0;\n");
          fprintf(out,"   while(in2.peek()!=EOF){\n");
          fprintf(out,"      %s%s.push_back(0);\n",prefix,name2var(member->name));
          snprintf(reference,sizeof(reference),"%s[i]",name2var(member->name));
          for(i=0;i<indent+3;i++) fputc(' ',out);
          s2c_gen_decode_c_simple_type(member->u.varray_.ref,prefix,reference,"in2",out);
          fprintf(out,"      i++;\n");
          fprintf(out,"   }\n;");
          fprintf(out,"   }\n");
        }
        else{
          /* Special case Data for Duane */
          fprintf(out,"   {\n");
          fprintf(out,"      UInt32 len;\n");
          fprintf(out,"      int c;\n");
          fprintf(out,"      decode_uintX(%s, %d, len);\n",
            instream, max2bytes(member->u.varray_.length)*8);
          fprintf(out,"      resip::DataStream strm(%s%s);\n",prefix,name2var(member->name));
          fprintf(out,"      while(len--){\n");
          fprintf(out,"        c=%s.get();\n",instream);
          fprintf(out,"        if(c==EOF)\n");
          fprintf(out,"          throw resip::ParseException(\"Premature end of data\",\n");
          fprintf(out,"          \"%s\",__FILE__,__LINE__);\n",member->name);          
          fprintf(out,"          strm.put(c);\n",name2var(member->name));
          fprintf(out,"      };\n");
          fprintf(out,"   }\n");
        }
        break;
      case TYPE_ARRAY:
        {
          char reference[100];
          int ct=(member->u.array_.length*8) / (member->u.array_.ref->u.primitive_.bits);
          
          fprintf(out,"   for(unsigned int i=0;i<%d;i++){\n",ct);
          snprintf(reference,sizeof(reference),"%s[i]",name2var(member->name));

          for(i=0;i<indent+3;i++) fputc(' ',out);
          s2c_gen_decode_c_simple_type(member->u.array_.ref,prefix,reference,instream,out);
          fprintf(out,"   }\n");
          break;
        }
      case TYPE_SELECT:
        {
          s2c_gen_decode_c_select(member, out, instream, 1);
        }
        break;
      default:
        nr_verr_exit("Don't know how to render element %s%s",prefix,member->name);
    }
    return(0);
  }

static int s2c_gen_decode_c_struct(p_decl *decl, FILE *out)
  {
    p_decl *entry;
    int do_auto=0;

    fprintf(out,"void %s :: decode(std::istream& in)\n{\n",type2class(decl->name));
    
    fprintf(out," StackLog(<< \"Decoding %s\");\n",type2class(decl->name));
    entry=STAILQ_FIRST(&decl->u.struct_.members);

    while(entry){
      if(entry->auto_len){
        if(do_auto)
          nr_verr_exit("Auto len feature can only be used once per struct");

        do_auto=entry->u.ref_.ref->u.primitive_.bits;
        fprintf(out,"   {\n");
        fprintf(out,"   resip::Data d;\n");
        fprintf(out,"   read_varray1(in, %d, d);\n",do_auto/8);
        fprintf(out,"   resip::DataStream in_auto(d);\n");
      }
      else{
        s2c_gen_decode_c_member(entry, out, do_auto?"in_auto":"in",0, "");
      }

      fprintf(out,"\n");
      entry=STAILQ_NEXT(entry,entry);

    }
    if(do_auto) {
      fprintf(out,"   if(in_auto.peek()!=EOF)\n");
      fprintf(out,"      throw resip::ParseException(\"Inner encoded value too long\",\n");
      fprintf(out,"      \"%s\",__FILE__,__LINE__);\n",decl->name);
      fprintf(out,"   }\n");
    }
    fprintf(out,"};\n\n");

    return(0);
  }



static int s2c_gen_construct_c_select(p_decl *decl, FILE *out, int do_inline)
  {
    p_decl *arm,*entry;
    char prefix[100];

    if(!do_inline){
      fprintf(out,"%s :: %s ()\n{\n",type2class(decl->name),
        type2class(decl->name));
      
      fprintf(out,"   StackLog(<< \"Constructing %s\");\n",type2class(decl->name));
      fprintf(out,"   mName = \"%s\";\n", type2class(decl->name));

    }

    arm=STAILQ_FIRST(&decl->u.select_.arms);
    while(arm){
      entry=STAILQ_FIRST(&arm->u.select_arm_.members);
      while(entry){
        snprintf(prefix,100,"m%s.",camelback(arm->name));
        s2c_gen_construct_c_member(entry, out, 9, prefix);
        entry=STAILQ_NEXT(entry,entry);
      }
      arm=STAILQ_NEXT(arm,entry);
    }
    if(!do_inline){
      fprintf(out,"};\n");
    }
    return(0);
  }



static int s2c_gen_construct_c_simple_type(p_decl *decl, char *prefix, char *reference, FILE *out)
  {
    if((decl->type == TYPE_REF) && (decl->u.ref_.ref->type==TYPE_ENUM))
      fprintf(out,"   %s%s=(%s)0;\n",prefix,reference,decl->u.ref_.ref->name);
    else
      fprintf(out,"   %s%s=0;\n",prefix,reference);




    return(0);
  }

static int s2c_gen_construct_c_member(p_decl *member, FILE *out,int indent, char *prefix)
  {
    int i;
    
    switch(member->type){
      case TYPE_REF:
        s2c_gen_construct_c_simple_type(member,prefix,name2var(member->name),out);
        break;
      case TYPE_VARRAY:
        {
          ;
        }
        break;
      case TYPE_ARRAY:
        {
          char reference[100];
          int ct=(member->u.array_.length*8) / (member->u.array_.ref->u.primitive_.bits);
          
          fprintf(out,"   for(unsigned int i=0;i<%d;i++){\n",ct);
          snprintf(reference,sizeof(reference),"%s%s[i]",prefix,name2var(member->name));

          for(i=0;i<indent+3;i++) fputc(' ',out);
          s2c_gen_construct_c_simple_type(member->u.array_.ref,"",reference,out);
          fprintf(out,"}\n");
        }
        break;
      case TYPE_SELECT:
        {
          s2c_gen_construct_c_select(member, out, 1);
        }
        break;
      default:
        nr_verr_exit("Don't know how to render element %s",member->name);
    }
  }

static int s2c_gen_construct_c_struct(p_decl *decl, FILE *out)
  {
    p_decl *entry;

    fprintf(out,"%s :: %s ()\n{\n",type2class(decl->name),type2class(decl->name));

    fprintf(out,"   mName = \"%s\";\n", type2class(decl->name));
    fprintf(out," StackLog(<< \"Constructing %s\");\n",type2class(decl->name));
    entry=STAILQ_FIRST(&decl->u.struct_.members);
    while(entry){
      if(!entry->auto_len){
        s2c_gen_construct_c_member(entry, out, 0, "");
      }
      
      fprintf(out,"\n");
      entry=STAILQ_NEXT(entry,entry);
    }

    fprintf(out,"};\n\n");

    return(0);    
  }

static int s2c_gen_copy_construct_c_all(p_decl *decl, FILE *out)
  {
    fprintf(out,"%s :: %s (const %s &from)\n{\n",type2class(decl->name),type2class(decl->name),
      type2class(decl->name));

    fprintf(out,"   // World's lamest copy constructor\n");
    fprintf(out,"   if(this==&from) return;\n");
    fprintf(out,"   mName = \"%s\";\n", type2class(decl->name));
    fprintf(out,"   resip::Data dat;\n");
    fprintf(out,"   {\n");
    fprintf(out,"     resip::DataStream strm(dat);\n");
    fprintf(out,"     from.encode(strm);\n");
    fprintf(out,"   }\n");
    fprintf(out,"   {\n");
    fprintf(out,"     resip::DataStream strm(dat);\n");
    fprintf(out,"     decode(strm);\n");
    fprintf(out,"   }\n");
    fprintf(out,"}\n\n");
  }

static int s2c_gen_pdu_c_struct(p_decl *decl, FILE *out)
  {
    fprintf(out,"\n\n// Classes for %s */\n\n",type2class(decl->name));

    s2c_gen_construct_c_struct(decl, out);
    s2c_gen_copy_construct_c_all(decl, out);
    s2c_gen_print_c_struct(decl, out);
    s2c_gen_decode_c_struct(decl, out);
    s2c_gen_encode_c_struct(decl, out);
    
    return(0);
  }


static int s2c_gen_print_c_select(p_decl *decl, FILE *out, int do_inline)
  {
    if(!do_inline){
      fprintf(out,"void %s :: print(std::ostream& out, int indent) const \n{\n",type2class(decl->name));
    }
    
    if(!do_inline){
      fprintf(out,"}\n");
    }
  }

static int s2c_gen_decode_c_select(p_decl *decl, FILE *out,char *instream, int do_inline)
  {
    p_decl *arm;
    p_decl *entry;
    char prefix[100];

    if(!do_inline){
      fprintf(out,"void %s :: decode(std::istream& in)\n{\n",type2class(decl->name));
    
      fprintf(out,"   StackLog(<< \"Decoding %s\");\n",type2class(decl->name));
    }

    fprintf(out,"   switch(%s){\n",name2var(decl->u.select_.switch_on));
    
    arm=STAILQ_FIRST(&decl->u.select_.arms);
    while(arm){
      fprintf(out,"      case %d:\n",arm->u.select_arm_.value);
      entry=STAILQ_FIRST(&arm->u.select_arm_.members);
      while(entry){
        snprintf(prefix,100,"m%s.",camelback(arm->name));
        s2c_gen_decode_c_member(entry, out, instream, 9, prefix);
        entry=STAILQ_NEXT(entry,entry);
      }
      fprintf(out,"          break;\n\n");
      arm=STAILQ_NEXT(arm,entry);
    }
    fprintf(out,"       default: /* User error */ \n");
    fprintf(out,"          assert(1==0);\n");
    fprintf(out,"   }\n\n");

    if(!do_inline){
      fprintf(out,"};\n");
    }

    return(0);
  }

static int s2c_gen_pdu_c_select(p_decl *decl, FILE *out)
  {
    fprintf(out,"\n\n// Classes for %s */\n\n",type2class(decl->name));
    
    s2c_gen_construct_c_select(decl, out, 0);
    s2c_gen_copy_construct_c_all(decl, out);
    s2c_gen_print_c_select(decl, out,0 );
    s2c_gen_decode_c_select(decl, out, "in" ,0);
    s2c_gen_encode_c_select(decl, out, 0);

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


