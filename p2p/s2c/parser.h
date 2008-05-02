/**
   parser.h

   Copyright (C) 2007, Network Resonance, Inc.
   All Rights Reserved.

   ekr@networkresonance.com  Tue Nov 13 16:18:32 2007
 */


#ifndef _parser_h
#define _parser_h

#include "r_common.h" 
#include "r_assoc.h"

#include <sys/queue.h>

typedef STAILQ_HEAD(p_decl_head_,p_decl_) p_decl_head;

typedef struct p_decl_ {
  char *name;
  int type;
#define TYPE_PRIMITIVE 1
#define TYPE_VARRAY     2
#define TYPE_STRUCT    3
#define TYPE_REF       4
#define TYPE_ENUM      5
#define TYPE_SELECT    6
#define TYPE_SELECT_ARM 7
#define TYPE_FWDREF     8
  union {
    struct {
      int bits;
    } primitive_;
    struct {
      int length;
      struct p_decl_ *ref;
    } varray_;
    struct {
      p_decl_head members;
    } struct_;
    struct {
      struct p_decl_ *ref;
    } ref_;
    struct {
      int value;
    } enum_;
    struct {
      p_decl_head arms;
    } select_;
    struct {
      int value;
      struct p_decl_ *ref;
    } select_arm_;
    struct {
      char *type;
    } fwd_ref_;
  } u;
  
  STAILQ_ENTRY(p_decl_) entry;
} p_decl;

extern r_assoc *types;


extern p_decl_head public_decls;

#endif

