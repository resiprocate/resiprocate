/**
   gen.c

   Copyright (C) 2008, RTFM, Inc.
   All Rights Reserved.

   ekr@rtfm.com  Fri May  2 22:12:12 2008
 */

//static char *RCSSTRING __UNUSED__="$Id$";

#include "parser.h"

static int s2c_gen_pdu_h_struct(p_decl *decl, FILE *out)
  {
    p_decl *entry;

    fprintf(out,"class %s : public PDU {\npublic:\n", decl->name);
    
    entry=STAILQ_FIRST(&decl->u.struct_.members);
    while(entry){
      switch(entry->type){
        case TYPE_REF:
          fprintf(out,"   %s %s;\n",entry->u.ref_.ref->name,entry->name);
          break;
        case TYPE_VARRAY:
          fprintf(out,"   vector<%s> %s\n",entry->u.varray_.ref->name,entry->name);
          break;
        default:
          nr_verr_exit("Don't know how to render element %s",entry->name);
      }
      entry=STAILQ_NEXT(entry,entry);
    }
    fprintf(out,"};\n\n\n");

    return(0);
  }


int s2c_gen_hdr_h(FILE *out)
  {
    fprintf(out,"#include \"s2c_native.hxx\"\nnamespace s2c {\n");
    
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
    else
      nr_verr_exit("Internal error: can't generate .h for PDU %s",decl->name);

    return(0);
  }

