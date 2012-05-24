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
#include <stdlib.h>
#include <string.h>
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
    char base_name[100]={0};
    int c;
    extern char *optarg;
    extern int optind;

    p_decl *decl;

    parser_preload();

    while((c=getopt(argc,argv,"b:"))!=-1){
      switch(c){
        case 'b':
          strcpy(base_name,optarg);
          break;
        default:
          nr_verr_exit("Bogus argument");
      }
    }
    
    argc-=optind;
    argv+=optind;

    /* Read the prologue */
    if(!(in=freopen("./decls.s2c","r",stdin))){
      nr_verr_exit("Couldn't read decls");
    }
    yyparse();
    fclose(in);
    yyset_lineno(1);

    if(argc==1){
      char *ptr,*sl=0;
      if(!(in=freopen(argv[0],"r",stdin)))
        nr_verr_exit("Couldn't open input file %s\n",argv[0]);

      if(strlen(base_name)==0){
        ptr=argv[0]+strlen(argv[0])-1;
        while(ptr >= argv[0]){
          if(*ptr=='/'){
            sl=ptr;
            break;
          }
          ptr--;
        }
        if(sl){
          sl++;
          strncpy(base_name,sl,sizeof(base_name));
        }
        else{
          strncpy(base_name,argv[0],sizeof(base_name));
        }

        if(strcmp(base_name + strlen(base_name)-4,".s2c"))
          nr_verr_exit("Wrong file type %s",base_name);
        base_name[strlen(base_name)-4]=0;
      }
    }
    else{
      nr_verr_exit("usage: s2c [-b base_name] <input-file>");
    }
    snprintf(nameh,sizeof(nameh),"%sGen.hxx",base_name);
    if(!(doth=fopen(nameh,"w")))
      nr_verr_exit("Couldn't open %s",nameh); 

    snprintf(namec,sizeof(namec),"%sGen.cxx",base_name);
    if(!(dotc=fopen(namec,"w")))
      nr_verr_exit("Couldn't open %s",namec);
    
    yyparse();

    decl=STAILQ_FIRST(&public_decls);
        
    s2c_gen_hdr_h(base_name,doth);
    s2c_gen_hdr_c(base_name,dotc);

    while(decl){
      s2c_gen_pdu_h(decl, doth);

      s2c_gen_pdu_c(decl, dotc);

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
