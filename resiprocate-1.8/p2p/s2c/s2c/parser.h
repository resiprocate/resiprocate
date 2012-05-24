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
  int auto_len;

#define TYPE_PRIMITIVE 1
#define TYPE_VARRAY     2
#define TYPE_STRUCT    3
#define TYPE_REF       4
#define TYPE_SELECT    6
#define TYPE_SELECT_ARM 7
#define TYPE_FWDREF     8
#define TYPE_ARRAY     9
#define TYPE_ENUM      10
#define TYPE_ENUM_VALUE 11
#define TYPE_OBJECT     12

  union {
    struct {
      char *type;
      int bits;
    } primitive_;
    struct {
      UINT4 length;
      struct p_decl_ *ref;
    } varray_;
    struct {
      p_decl_head members;
    } struct_;
    struct {
      struct p_decl_ *ref;
    } ref_;
    struct {
      char *switch_on;
      p_decl_head arms;
    } select_;
    struct {
      int value;
      p_decl_head members;
    } select_arm_;
    struct {
      char *type;
    } fwd_ref_;
    struct {
      UINT4 length;
      struct p_decl_ *ref;
    } array_;
    struct {
      int max;
      p_decl_head members;
    } enum_;
    struct {
      int value;
    } enum_value_;
    struct {
      char *classname;
    } object_;
  } u;
  
  STAILQ_ENTRY(p_decl_) entry;
} p_decl;

extern r_assoc *types;


extern p_decl_head public_decls;

#endif

