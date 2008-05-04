/**
   main.c


   Copyright (C) 1999-2000 RTFM, Inc.
   All Rights Reserved

   This package is a SSLv3/TLS protocol analyzer written by Eric Rescorla
   <ekr@rtfm.com> and licensed by RTFM, Inc.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. All advertising materials mentioning features or use of this software
      must display the following acknowledgement:
   
      This product includes software developed by Eric Rescorla for
      RTFM, Inc.

   4. Neither the name of RTFM, Inc. nor the name of Eric Rescorla may be
      used to endorse or promote products derived from this
      software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY ERIC RESCORLA AND RTFM, INC. ``AS IS'' AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY SUCH DAMAGE.

   $Id: main.c,v 1.2 2000/10/17 16:10:01 ekr Exp $


   ekr@rtfm.com  Mon Jan 18 16:28:43 1999
 */


static char *RCSSTRING="$Id: main.c,v 1.2 2000/10/17 16:10:01 ekr Exp $";

extern int yydebug;

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include "parser.h"
#include "gen.h"
#include "r_log.h"

extern int yydebug;

FILE *doth,*dotc;

r_assoc *types;
p_decl_head public_decls;

int parser_preload()
  {
    int r;
    
    r_assoc_create(&types,r_assoc_crc32_hash_compute,10);
    STAILQ_INIT(&public_decls);
  }


int main(argc,argv)
  int argc;
  char **argv;
  {
    char namec[100],nameh[100];
    FILE *in;
    FILE *dotc;
    FILE *doth;

    p_decl *decl;

    parser_preload();
    
    if(argc==1){
      ;
    }
    if(argc==2){
      if(!(in=freopen(argv[1],"r",stdin)))
      nr_verr_exit("Couldn't open input file %s\n",argv[1]);
    }
    else{
      fprintf(stderr,"usage: s2c <input-file>");
    }
    snprintf(nameh,sizeof(nameh),"%s.hxx",argv[1]);
    if(!(doth=fopen(nameh,"w")))
      nr_verr_exit("Couldn't open %s",nameh); 

    snprintf(namec,sizeof(namec),"%s.cxx",argv[1]);
    if(!(dotc=fopen(namec,"w")))
      nr_verr_exit("Couldn't open %s",namec);
    
    yyparse();

    decl=STAILQ_FIRST(&public_decls);
        
    s2c_gen_hdr_h(argv[1],doth);
    s2c_gen_hdr_c(argv[1],dotc);

    while(decl){
      s2c_gen_pdu_h(decl, doth);

      decl=STAILQ_NEXT(decl,entry);
    }

    s2c_gen_ftr_h(doth);
    s2c_gen_ftr_c(dotc);

    exit(0);
  }


extern int yylineno;

int yywrap()
{
;}

int yyerror(s)
  char *s;
  {
    printf("Parse error %s at line %d\n",s,yylineno);
    exit(1);
  }
