/**
   gen.h

   Copyright (C) 2008, RTFM, Inc.
   All Rights Reserved.

   ekr@rtfm.com  Fri May  2 22:47:16 2008
 */


#ifndef _gen_h
#define _gen_h

int s2c_gen_hdr_h(char *name, FILE *out);
int s2c_gen_ftr_h(FILE *out);
int s2c_gen_hdr_c(char *name,FILE *out);
int s2c_gen_ftr_c(FILE *out);
int s2c_gen_pdu_h(p_decl *decl, FILE *out);


#endif


