/*
 * FILE : sendOSC_wrap.c
 * 
 * This file was automatically generated by :
 * Simplified Wrapper and Interface Generator (SWIG)
 * Version 1.1 (Build 883)
 * 
 * Portions Copyright (c) 1995-1998
 * The University of Utah and The Regents of the University of California.
 * Permission is granted to distribute this file in any manner provided
 * this notice remains intact.
 * 
 * Do not make changes to this file--changes will be lost!
 *
 */


#define SWIGCODE
/* $Header: /usr/local/cvsroot/slub/clients/OSC_perl/sendOSC_wrap.c,v 1.1 2003-10-30 20:34:56 alex Exp $ */
/* Implementation : PERL 5 */

#define SWIGPERL
#define SWIGPERL5
#ifdef __cplusplus
/* Needed on some windows machines---since MS plays funny
   games with the header files under C++ */
#include <math.h>
#include <stdlib.h>
extern "C" {
#endif
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

/* Get rid of free and malloc defined by perl */
#undef free
#undef malloc

#include <string.h>
#ifdef __cplusplus
}
#endif
/* Definitions for compiling Perl extensions on a variety of machines */

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#   if defined(_MSC_VER)
#	define SWIGEXPORT(a) __declspec(dllexport) a
#   else
#	if defined(__BORLANDC__)
#	    define SWIGEXPORT(a) a _export 
#	else
#	    define SWIGEXPORT(a) a 
#	endif
#   endif
#else
#   define SWIGEXPORT(a) a 
#endif

#ifdef PERL_OBJECT
#define MAGIC_PPERL  CPerlObj *pPerl = (CPerlObj *) this;
#define MAGIC_CAST   (int (CPerlObj::*)(SV *, MAGIC *))
#define SWIGCLASS_STATIC 
#else
#define MAGIC_PPERL
#define MAGIC_CAST
#define SWIGCLASS_STATIC static
#endif

#if defined(WIN32) && defined(PERL_OBJECT) && !defined(PerlIO_exportFILE)
#define PerlIO_exportFILE(fh,fl) (FILE*)(fh)
#endif

/* Modifications for newer Perl 5.005 releases */

#if !defined(PERL_REVISION) || ((PERL_REVISION >= 5) && ((PERL_VERSION < 5) || ((PERL_VERSION == 5) && (PERL_SUBVERSION < 50))))
#ifndef PL_sv_yes
#define PL_sv_yes sv_yes
#endif
#ifndef PL_sv_undef
#define PL_sv_undef sv_undef
#endif
#ifndef PL_na
#define PL_na na
#endif
#endif

/******************************************************************************
 * Pointer type-checking code
 *****************************************************************************/

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SWIG_NOINCLUDE
extern void SWIG_MakePtr(char *, void *, char *);
#ifndef PERL_OBJECT
extern void SWIG_RegisterMapping(char *, char *, void *(*)(void *));
#else
#define SWIG_RegisterMapping(a,b,c) _SWIG_RegisterMapping(pPerl,a,b,c);
extern void _SWIG_RegisterMapping(CPerlObj *,char *, char *, void *(*)(void *),int);
#endif
#ifndef PERL_OBJECT
extern char *SWIG_GetPtr(SV *, void **, char *);
#else
extern char *_SWIG_GetPtr(CPerlObj *, SV *, void **, char *);
#define SWIG_GetPtr(a,b,c) _SWIG_GetPtr(pPerl,a,b,c)
#endif

#else

#ifdef SWIG_GLOBAL
#define SWIGSTATICRUNTIME(a) SWIGEXPORT(a)
#else
#define SWIGSTATICRUNTIME(a) static a
#endif

/* These are internal variables.   Should be static */

typedef struct SwigPtrType {
  char               *name;
  int                 len;
  void               *(*cast)(void *);
  struct SwigPtrType *next;
} SwigPtrType;

/* Pointer cache structure */

typedef struct {
  int                 stat;               /* Status (valid) bit             */
  SwigPtrType        *tp;                 /* Pointer to type structure      */
  char                name[256];          /* Given datatype name            */
  char                mapped[256];        /* Equivalent name                */
} SwigCacheType;

static int SwigPtrMax  = 64;           /* Max entries that can be currently held */
static int SwigPtrN    = 0;            /* Current number of entries              */
static int SwigPtrSort = 0;            /* Status flag indicating sort            */
static SwigPtrType *SwigPtrTable = 0;  /* Table containing pointer equivalences  */
static int SwigStart[256];             /* Table containing starting positions    */

/* Cached values */

#define SWIG_CACHESIZE  8
#define SWIG_CACHEMASK  0x7
static SwigCacheType SwigCache[SWIG_CACHESIZE];  
static int SwigCacheIndex = 0;
static int SwigLastCache = 0;

/* Sort comparison function */
static int swigsort(const void *data1, const void *data2) {
	SwigPtrType *d1 = (SwigPtrType *) data1;
	SwigPtrType *d2 = (SwigPtrType *) data2;
	return strcmp(d1->name,d2->name);
}

/* Binary Search function */
static int swigcmp(const void *key, const void *data) {
  char *k = (char *) key;
  SwigPtrType *d = (SwigPtrType *) data;
  return strncmp(k,d->name,d->len);
}

/* Register a new datatype with the type-checker */

#ifndef PERL_OBJECT
SWIGSTATICRUNTIME(void) 
SWIG_RegisterMapping(char *origtype, char *newtype, void *(*cast)(void *)) {
#else
#define SWIG_RegisterMapping(a,b,c) _SWIG_RegisterMapping(pPerl, a,b,c)
SWIGSTATICRUNTIME(void)
_SWIG_RegisterMapping(CPerlObj *pPerl, char *origtype, char *newtype, void *(*cast)(void *)) {
#endif

  int i;
  SwigPtrType *t = 0, *t1;

  if (!SwigPtrTable) {     
    SwigPtrTable = (SwigPtrType *) malloc(SwigPtrMax*sizeof(SwigPtrType));
    SwigPtrN = 0;
  }
  if (SwigPtrN >= SwigPtrMax) {
    SwigPtrMax = 2*SwigPtrMax;
    SwigPtrTable = (SwigPtrType *) realloc(SwigPtrTable,SwigPtrMax*sizeof(SwigPtrType));
  }
  for (i = 0; i < SwigPtrN; i++)
    if (strcmp(SwigPtrTable[i].name,origtype) == 0) {
      t = &SwigPtrTable[i];
      break;
    }
  if (!t) {
    t = &SwigPtrTable[SwigPtrN];
    t->name = origtype;
    t->len = strlen(t->name);
    t->cast = 0;
    t->next = 0;
    SwigPtrN++;
  }
  while (t->next) {
    if (strcmp(t->name,newtype) == 0) {
      if (cast) t->cast = cast;
      return;
    }
    t = t->next;
  }
  t1 = (SwigPtrType *) malloc(sizeof(SwigPtrType));
  t1->name = newtype;
  t1->len = strlen(t1->name);
  t1->cast = cast;
  t1->next = 0;
  t->next = t1;
  SwigPtrSort = 0;
}

/* Make a pointer value string */

SWIGSTATICRUNTIME(void) 
SWIG_MakePtr(char *_c, const void *_ptr, char *type) {
  static char _hex[16] =
  {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   'a', 'b', 'c', 'd', 'e', 'f'};
  unsigned long _p, _s;
  char _result[20], *_r;    /* Note : a 64-bit hex number = 16 digits */
  _r = _result;
  _p = (unsigned long) _ptr;
  if (_p > 0) {
    while (_p > 0) {
      _s = _p & 0xf;
      *(_r++) = _hex[_s];
      _p = _p >> 4;
    }
    *_r = '_';
    while (_r >= _result)
      *(_c++) = *(_r--);
  } else {
    strcpy (_c, "NULL");
  }
  if (_ptr)
    strcpy (_c, type);
}

/* Function for getting a pointer value */

#ifndef PERL_OBJECT
SWIGSTATICRUNTIME(char *) 
SWIG_GetPtr(SV *sv, void **ptr, char *_t)
#else
#define SWIG_GetPtr(a,b,c) _SWIG_GetPtr(pPerl,a,b,c)
SWIGSTATICRUNTIME(char *)
_SWIG_GetPtr(CPerlObj *pPerl, SV *sv, void **ptr, char *_t)
#endif
{
  char temp_type[256];
  char *name,*_c;
  int  len,i,start,end;
  IV   tmp;
  SwigPtrType *sp,*tp;
  SwigCacheType *cache;

  /* If magical, apply more magic */

  if (SvGMAGICAL(sv))
    mg_get(sv);

  /* Check to see if this is an object */
  if (sv_isobject(sv)) {
    SV *tsv = (SV*) SvRV(sv);
    if ((SvTYPE(tsv) == SVt_PVHV)) {
      MAGIC *mg;
      if (SvMAGICAL(tsv)) {
	mg = mg_find(tsv,'P');
	if (mg) {
	  SV *rsv = mg->mg_obj;
	  if (sv_isobject(rsv)) {
	    tmp = SvIV((SV*)SvRV(rsv));
	  }
	}
      } else {
	return "Not a valid pointer value";
      }
    } else {
      tmp = SvIV((SV*)SvRV(sv));
    }
    if (!_t) {
      *(ptr) = (void *) tmp;
      return (char *) 0;
    }
  } else if (! SvOK(sv)) {            /* Check for undef */
    *(ptr) = (void *) 0;
    return (char *) 0;
  } else if (SvTYPE(sv) == SVt_RV) {       /* Check for NULL pointer */
    *(ptr) = (void *) 0;
    if (!SvROK(sv)) 
      return (char *) 0;
    else
      return "Not a valid pointer value";
  } else {                                 /* Don't know what it is */
      *(ptr) = (void *) 0;
      return "Not a valid pointer value";
  }
  if (_t) {
    /* Now see if the types match */      

    if (!sv_isa(sv,_t)) {
      _c = HvNAME(SvSTASH(SvRV(sv)));
      if (!SwigPtrSort) {
	qsort((void *) SwigPtrTable, SwigPtrN, sizeof(SwigPtrType), swigsort);  
	for (i = 0; i < 256; i++) {
	  SwigStart[i] = SwigPtrN;
	}
	for (i = SwigPtrN-1; i >= 0; i--) {
	  SwigStart[SwigPtrTable[i].name[0]] = i;
	}
	for (i = 255; i >= 1; i--) {
	  if (SwigStart[i-1] > SwigStart[i])
	    SwigStart[i-1] = SwigStart[i];
	}
	SwigPtrSort = 1;
	for (i = 0; i < SWIG_CACHESIZE; i++)  
	  SwigCache[i].stat = 0;
      }
      /* First check cache for matches.  Uses last cache value as starting point */
      cache = &SwigCache[SwigLastCache];
      for (i = 0; i < SWIG_CACHESIZE; i++) {
	if (cache->stat) {
	  if (strcmp(_t,cache->name) == 0) {
	    if (strcmp(_c,cache->mapped) == 0) {
	      cache->stat++;
	      *ptr = (void *) tmp;
	      if (cache->tp->cast) *ptr = (*(cache->tp->cast))(*ptr);
	      return (char *) 0;
	    }
	  }
	}
	SwigLastCache = (SwigLastCache+1) & SWIG_CACHEMASK;
	if (!SwigLastCache) cache = SwigCache;
	else cache++;
      }

      start = SwigStart[_t[0]];
      end = SwigStart[_t[0]+1];
      sp = &SwigPtrTable[start];
      while (start < end) {
	if (swigcmp(_t,sp) == 0) break;
	sp++;
	start++;
      }
      if (start > end) sp = 0;
      while (start <= end) {
	if (swigcmp(_t,sp) == 0) {
	  name = sp->name;
	  len = sp->len;
	  tp = sp->next;
	  while(tp) {
	    if (tp->len >= 255) {
	      return _c;
	    }
	    strcpy(temp_type,tp->name);
	    strncat(temp_type,_t+len,255-tp->len);
	    if (sv_isa(sv,temp_type)) {
	      /* Get pointer value */
	      *ptr = (void *) tmp;
	      if (tp->cast) *ptr = (*(tp->cast))(*ptr);

	      strcpy(SwigCache[SwigCacheIndex].mapped,_c);
	      strcpy(SwigCache[SwigCacheIndex].name,_t);
	      SwigCache[SwigCacheIndex].stat = 1;
	      SwigCache[SwigCacheIndex].tp = tp;
	      SwigCacheIndex = SwigCacheIndex & SWIG_CACHEMASK;
	      return (char *) 0;
	    }
	    tp = tp->next;
	  } 
	}
	sp++;
	start++;
      }
      /* Didn't find any sort of match for this data.  
	 Get the pointer value and return the received type */
      *ptr = (void *) tmp;
      return _c;
    } else {
      /* Found a match on the first try.  Return pointer value */
      *ptr = (void *) tmp;
      return (char *) 0;
    }
  } 
  *ptr = (void *) tmp;
  return (char *) 0;
}

#endif
#ifdef __cplusplus
}
#endif






/* Magic variable code */
#ifndef PERL_OBJECT
#define swig_create_magic(s,a,b,c) _swig_create_magic(s,a,b,c)
static void _swig_create_magic(SV *sv, char *name, int (*set)(SV *, MAGIC *), int (*get)(SV *,MAGIC *)) {
#else
#define swig_create_magic(s,a,b,c) _swig_create_magic(pPerl,s,a,b,c)
static void _swig_create_magic(CPerlObj *pPerl, SV *sv, char *name, int (CPerlObj::*set)(SV *, MAGIC *), int (CPerlObj::*get)(SV *, MAGIC *)) {
#endif
  MAGIC *mg;
  sv_magic(sv,sv,'U',name,strlen(name));
  mg = mg_find(sv,'U');
  mg->mg_virtual = (MGVTBL *) malloc(sizeof(MGVTBL));
  mg->mg_virtual->svt_get = get;
  mg->mg_virtual->svt_set = set;
  mg->mg_virtual->svt_len = 0;
  mg->mg_virtual->svt_clear = 0;
  mg->mg_virtual->svt_free = 0;
}

#define SWIG_init    boot_sendOSC

#define SWIG_name   "sendOSC::boot_sendOSC"
#define SWIG_varinit "sendOSC::var_sendOSC_init();"
#ifdef __cplusplus
extern "C"
#endif
#ifndef PERL_OBJECT
SWIGEXPORT(void) boot_sendOSC(CV* cv);
#else
SWIGEXPORT(void) boot_sendOSC(CV *cv, CPerlObj *);
#endif

/* bla */
#include "OSC-client.h"
#include "htmsocket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int print_args(char **argv) {
    int i = 0;
    while (argv[i]) {
         printf("argv[%d] = %s\n", i,argv[i]);
         i++;
    }
    return i;
}

// Returns a char ** list 

char **get_args() {
    static char *values[] = { "Dave", "Mike", "Susan", "John", "Michelle", 0};
    return &values[0];
}


extern void HartCoreMode(int ,char *[]);
#ifdef PERL_OBJECT
#define MAGIC_CLASS _wrap_sendOSC_var::
class _wrap_sendOSC_var : public CPerlObj {
public:
#else
#define MAGIC_CLASS
#endif
SWIGCLASS_STATIC int swig_magic_readonly(SV *sv, MAGIC *mg) {
    MAGIC_PPERL
    sv = sv; mg = mg;
    croak("Value is read-only.");
    return 0;
}


#ifdef PERL_OBJECT
};
#endif

XS(_wrap_print_args) {

    int  _result;
    char ** _arg0;
    int argvi = 0;
    SV * _saved[1];
    dXSARGS ;

    cv = cv;
    if ((items < 1) || (items > 1)) 
        croak("Usage: print_args(argv);");
{
	AV *tempav;
	I32 len;
	int i;
	SV  **tv;

	if (!SvROK(ST(0)))
	    croak("ST(0) is not an array.");
        if (SvTYPE(SvRV(ST(0))) != SVt_PVAV)
	    croak("ST(0) is not an array.");
        tempav = (AV*)SvRV(ST(0));
	len = av_len(tempav);
	_arg0 = (char **) malloc((len+2)*sizeof(char *));
	for (i = 0; i <= len; i++) {
	    tv = av_fetch(tempav, i, 0);	
	    _arg0[i] = (char *) SvPV(*tv,PL_na);
        }
	_arg0[i] = 0;
}
    _saved[0] = ST(0);
    _result = (int )print_args(_arg0);
    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
{
	free(_arg0);
}
    XSRETURN(argvi);
}

XS(_wrap_get_args) {

    char ** _result;
    int argvi = 0;
    dXSARGS ;

    cv = cv;
    if ((items < 0) || (items > 0)) 
        croak("Usage: get_args();");
    _result = (char **)get_args();
{
	AV *myav;
	SV **svs;
	int i = 0,len = 0;
	/* Figure out how many elements we have */
	while (_result[len])
	   len++;
	svs = (SV **) malloc(len*sizeof(SV *));
	for (i = 0; i < len ; i++) {
	    svs[i] = sv_newmortal();
	    sv_setpv((SV*)svs[i],_result[i]);
	};
	myav =	av_make(len,svs);
	for (i = 0; i < len; i++) {
	  /*	  SvREFCNT_dec(svs[i]); */
	}
	free(svs);
        ST(argvi) = newRV((SV*)myav);
        sv_2mortal(ST(argvi));
	argvi++;                      /* This is critical! */
}
    XSRETURN(argvi);
}

XS(_wrap_HartCoreMode) {

    int  _arg0;
    char ** _arg1;
    int argvi = 0;
    SV * _saved[1];
    dXSARGS ;

    cv = cv;
    if ((items < 2) || (items > 2)) 
        croak("Usage: HartCoreMode(argc,argv);");
    _arg0 = (int )SvIV(ST(0));
{
	AV *tempav;
	I32 len;
	int i;
	SV  **tv;

	if (!SvROK(ST(1)))
	    croak("ST(1) is not an array.");
        if (SvTYPE(SvRV(ST(1))) != SVt_PVAV)
	    croak("ST(1) is not an array.");
        tempav = (AV*)SvRV(ST(1));
	len = av_len(tempav);
	_arg1 = (char **) malloc((len+2)*sizeof(char *));
	for (i = 0; i <= len; i++) {
	    tv = av_fetch(tempav, i, 0);	
	    _arg1[i] = (char *) SvPV(*tv,PL_na);
        }
	_arg1[i] = 0;
}
    _saved[0] = ST(1);
    HartCoreMode(_arg0,_arg1);
{
	free(_arg1);
}
    XSRETURN(argvi);
}

/*
 * This table is used by the pointer type-checker
 */
static struct { char *n1; char *n2; void *(*pcnv)(void *); } _swig_mapping[] = {
    { "unsigned short","short",0},
    { "long","unsigned long",0},
    { "long","signed long",0},
    { "signed short","short",0},
    { "signed int","int",0},
    { "short","unsigned short",0},
    { "short","signed short",0},
    { "unsigned long","long",0},
    { "int","unsigned int",0},
    { "int","signed int",0},
    { "unsigned int","int",0},
    { "signed long","long",0},
{0,0,0}};

XS(_wrap_perl5_sendOSC_var_init) {
    dXSARGS;
    SV *sv;
    cv = cv; items = items;
    XSRETURN(1);
}
#ifdef __cplusplus
extern "C"
#endif
XS(boot_sendOSC) {
	 dXSARGS;
	 char *file = __FILE__;
	 cv = cv; items = items;
	 newXS("sendOSC::var_sendOSC_init", _wrap_perl5_sendOSC_var_init, file);
	 newXS("sendOSC::print_args", _wrap_print_args, file);
	 newXS("sendOSC::get_args", _wrap_get_args, file);
	 newXS("sendOSC::HartCoreMode", _wrap_HartCoreMode, file);
{
   int i;
   for (i = 0; _swig_mapping[i].n1; i++)
        SWIG_RegisterMapping(_swig_mapping[i].n1,_swig_mapping[i].n2,_swig_mapping[i].pcnv);
}
	 ST(0) = &PL_sv_yes;
	 XSRETURN(1);
}
