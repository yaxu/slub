


/*
#    Sfront, a SAOL to C translator    
#    This file: Included file in sfront runtime
#    Copyright (C) 1999  Regents of the University of California
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License (Version 2) as
#    published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#    Primary Author: John Lazzaro, lazzaro@cs.berkeley.edu
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

/********************************/
/* readabiliy-improving defines */
/********************************/

#define NV(x)   nstate->v[x].f
#define NVI(x)  nstate->v[x].i
#define NVU(x)  nstate->v[x]
#define NT(x)   nstate->t[x]
#define NS(x)   nstate->x
#define NSP     nstate
#define NP(x)   nstate->v[x].f
#define NPI(x)  nstate->v[x].i
#define NG(x)   global[x].f
#define NGI(x)  global[x].i
#define NGU(x)  global[x]

#define TB(x)   bus[x]
#define STB(x)  sbus[x]
#define ROUND(x) ( ((x) > 0.0F) ? ((int) ((x) + 0.5F)) :  ((int) ((x) - 0.5F)))
#define POS(x)   (((x) > 0.0F) ? x : 0.0F)
#define RMULT ((float)(1.0F/(RAND_MAX + 1.0F)))

#define NOTUSEDYET 0
#define TOBEPLAYED 1
#define PAUSED     2
#define PLAYING    3
#define ALLDONE    4

#define NOTLAUNCHED 0
#define LAUNCHED 1

#define ASYS_DONE        0
#define ASYS_EXIT        1
#define ASYS_ERROR       2

#define IPASS 1
#define KPASS 2
#define APASS 3

#define IOERROR_RETRY 256 

/************************************/
/* externs for system functions     */
/************************************/

extern void epr(int, char *, char *, char *);
extern size_t rread(void * ptr, size_t len, size_t nmemb, FILE * stream);
extern size_t rwrite(void * ptr, size_t len, size_t nmemb, FILE * stream);

/************************************/
/*  union for a data stack element  */
/************************************/

typedef union {

float f;
long  i;

} dstack;


/************************************/
/* ntables: table entries for notes */
/************************************/

typedef struct tableinfo {

int    len;                /* length of table */
float  lenf;               /* length of table, as a float */

int    start;              /* loop start position */
int    end;                /* loop end position */
float  sr;                 /* table sampling rate  */
float  base;               /* table base frequency */

                           /* precomputed constants       */
int tend;                  /* len -1 if end==0            */
float m;                   /* start/len                   */
float n;                   /* end/len, if end=0 1-(1/len) */
float dmult;               /* sr*atime                    */
float lmult;               /* (sr*atime)/basefreq         */
float diff;                /* n - m                       */

float  *t;                 /* pointer to table entries    */
float stamp;               /* timestamp on table contents */
char llmem;                /* 1 if *t was malloced        */
} tableinfo; 

/********************/
/*  control lines   */
/********************/

typedef struct scontrol_lines {

float t;                  /* trigger time */
int label;                /* index into label array */
int siptr;                /* score instr line to control */
struct instr_line *iline; /* pointer to score line */
int imptr;                /* position of variable in v[] */
float imval;              /* value to import into v[] */

} scontrol_lines;

/********************/
/*   tempo lines    */
/********************/

typedef struct stempo_lines {

  float t;          /* trigger time */
  float newtempo;   /* new tempo */ 

} stempo_lines;

/********************/
/*   table lines    */
/********************/

typedef struct stable_lines {

  float t;          /* trigger time */
  int gindex;       /* global table to target */
  int size;         /* size of data */
  void (*tmake) (); /* function   */
  void * data;      /* data block */

} stable_lines;

/********************/
/* system variables */
/********************/

/* audio and control rates */

float globaltune = 440.0F;
float invglobaltune = 2.272727e-03F;
float scorebeats = 0.0F;              /* current score beat */
float absolutetime = 0.0F;            /* current absolute time */
int kbase = 1;                        /* kcycle of last tempo change */
float scorebase = 0.0F;               /* scorebeat of last tempo change */

/* counters & bounds acycles and kcycles */

int endkcycle;
int kcycleidx = 1;
int acycleidx = 0;
int pass = IPASS;
int beginflag;
sig_atomic_t graceful_exit;

struct instr_line * sysidx;

int busidx;        /* counter for buses                   */
int nextstate = 0; /* counter for active instrument state */
int oldstate;      /* detects loops in nextstate updates  */
int tstate;        /* flag for turnoff state machine      */
float cpuload;     /* current cpu-time value              */

int asys_argc;
char ** asys_argv;

int csys_argc;
char ** csys_argv;


int csys_sfront_argc = 9;

char * csys_sfront_argv[9] = {
"sfront",
"-aout",
"linux",
"-cin",
"ascii",
"-orc",
"ascii.saol",
"-sco",
"ascii.sasl"};


#define APPNAME "sfront"
#define APPVERSION "0.85 10/13/02"
#define NSYS_NET 0
#define ARATE 44100.0F
#define ATIME 2.267574e-05F
#define KRATE 1050.0F
#define KTIME 9.523810e-04F
#define KMTIME 9.523810e-01F
#define KUTIME 952L
#define ACYCLE 42L
float tempo = 60.0F;
float scoremult = 9.523810e-04F;

#define ASYS_RENDER   0
#define ASYS_PLAYBACK 1
#define ASYS_TIMESYNC 2

#define ASYS_SHORT   0
#define ASYS_FLOAT   1

/* audio i/o */

#define ASYS_OCHAN 1L
#define ASYS_OTYPENAME  ASYS_SHORT
#define ASYS_OTYPE  short
long asys_osize = 0;
long obusidx = 0;

ASYS_OTYPE * asys_obuf = NULL;

#define ASYS_USERLATENCY  0
#define ASYS_LOWLATENCY   0
#define ASYS_HIGHLATENCY  1
#define ASYS_LATENCYTYPE  ASYS_LOWLATENCY
#define ASYS_LATENCY 0.002000F
#define ASYS_TIMEOPTION ASYS_PLAYBACK

#define MAXPFIELDS 2

struct instr_line { 

float starttime;  /* score start time of note */
float endtime;    /* score end time of note */
float startabs;   /* absolute start time of note */
float endabs;     /* absolute end time of note */
float abstime;    /* absolute time extension */
float time;       /* time of note start (absolute) */
float itime;      /* elapsed time (absolute) */
float sdur;       /* duration of note in score time*/

int kbirth;       /* kcycleidx for note launch */
int released;     /* flag for turnoff*/
int turnoff;      /* flag for turnoff */
int noteon;       /* NOTYETUSED,TOBEPLAYED,PAUSED,PLAYING,ALLDONE */
int notestate;    /* index into state array */
int launch;       /* only for dynamic instruments */
int numchan;      /* only for MIDI notes */
int preset;       /* only for MIDI notes */
int notenum;      /* only for MIDI notes */
int label;        /* SASL label index + 1, or 0 (no label) */

                  /* for static MIDI, all-sounds noteoff */

float p[MAXPFIELDS];          /* parameters */

struct ninstr_types * nstate; /* pointer into state array */

};

#define BUS_output_bus 0
#define ENDBUS_output_bus 1
#define ENDBUS 1
float bus[ENDBUS];

#define CSYS_EXTCHANSTART 0
int midimasterchannel = CSYS_EXTCHANSTART;

/* first 4144 locations for MIDI */
#define GBL_STARTVAR 4144
#define GBL_ENDVAR 4144
/* global variables */

dstack global[GBL_ENDVAR+1];

#define GBL_ENDTBL 0
struct tableinfo gtables[GBL_ENDTBL+1];

#define sine_pitch 0
#define sine_vel 1
#define sine_a 2
#define sine_attack 3
#define sine_release 4
#define sine_attlim 5
#define sine_ktime 6
#define sine_stime 7
#define sine_incr 8
#define sine_rel 9
#define sine_tot 10
#define sine_x 11
#define sine_y 12
#define sine_init 13
#define sine_env 14
#define sine__tvr1 15
#define sine__tvr0 16
#define sine_int1_return 17
#define sine_int2_return 18
#define sine_cpsmidi3_return 19
#define sine_sin4_return 20
#define sine_ENDVAR 21

#define sine_ENDTBL 0

extern void sigint_handler(int);

void sine_ipass(struct ninstr_types *);
void sine_kpass(struct ninstr_types *);
void sine_apass(struct ninstr_types *);




#define CSYS_GIVENENDTIME 1

#define MAXENDTIME 1E+37

float endtime = 86400.0F;
#define MAXCNOTES 256

#define CSYS_INSTRNUM 1

#define CSYS_IRATE 0
#define CSYS_KRATE 1
#define CSYS_ARATE 2
#define CSYS_TABLE 3

#define CSYS_NORMAL 0
#define CSYS_IMPORT 1
#define CSYS_EXPORT 2
#define CSYS_IMPORTEXPORT 3
#define CSYS_PFIELD 4
#define CSYS_INTERNAL 5

#define CSYS_STATUS_EFFECTS    1 
#define CSYS_STATUS_SCORE      2 
#define CSYS_STATUS_MIDI       4 
#define CSYS_STATUS_DYNAMIC    8 
#define CSYS_STATUS_STARTUP   16 

typedef struct csys_varstruct {
  int index;
  char * name;
  int token;
  int type;
  int tag;
  int width;
} csys_varstruct;

typedef struct csys_instrstruct {
  int index;
  char * name;
  int token;
  int numvars;
  csys_varstruct * vars;
  int outwidth;
  int status;
} csys_instrstruct;

typedef struct csys_labelstruct {
  int index;
  char * name;
  int token;
  int iflag[CSYS_INSTRNUM];
} csys_labelstruct;

typedef struct csys_presetstruct {
  int index;
  int preset;
} csys_presetstruct;

typedef struct csys_samplestruct {
  int index;
  int token;
  char * name;
  char * fname;
} csys_samplestruct;

typedef struct csys_routestruct {
  int bus;
  int ninstr;
  int * instr;
} csys_routestruct;

typedef struct csys_sendstruct {
  int instr;
  int nbus;
  int * bus;
} csys_sendstruct;

typedef struct csys_busstruct {
  int index;
  char * name;
  int width;
  int oflag;
} csys_busstruct;

typedef struct csys_targetstruct {
char * name;
int token;
int numinstr;
int * instrindex;
int * varindex;
} csys_targetstruct;

 float position[3];
 float direction[3];
 float listenerPosition[3];
 float listenerDirection[3];
 float minFront;
 float maxFront;
 float minBack;
 float maxBack;
 float params[128];


void csys_terminate(char * message)
{
   epr(0,NULL,"ascii control driver", message);
}

#define CSYS_GLOBALNUM 0

csys_varstruct csys_global[1];

#define CSYS_TARGETNUM 0

csys_targetstruct csys_target[1];

#define CSYS_NOLABEL 0

#define CSYS_LABELNUM 0

csys_labelstruct csys_labels[1];

#define CSYS_PRESETNUM 1

csys_presetstruct csys_presets[CSYS_PRESETNUM] = {
0, 0 };

#define CSYS_SAMPLENUM 0

csys_samplestruct csys_samples[1];

#define CSYS_BUSNUM 1

csys_busstruct csys_bus[CSYS_BUSNUM] = {
0, "output_bus",1, 0 };

#define CSYS_ROUTENUM 0

csys_routestruct csys_route[1];

#define CSYS_SENDNUM 0

csys_sendstruct csys_send[1];

#define CSYS_sine_VARNUM 15

csys_varstruct csys_sine_vars[CSYS_sine_VARNUM] = {
0, "pitch", 1, CSYS_IRATE, CSYS_PFIELD, 1 ,
0, "vel", 2, CSYS_IRATE, CSYS_PFIELD, 1 ,
0, "a", 3, CSYS_IRATE, CSYS_NORMAL, 1 ,
1, "attack", 4, CSYS_IRATE, CSYS_NORMAL, 1 ,
1, "release", 5, CSYS_IRATE, CSYS_NORMAL, 1 ,
1, "attlim", 6, CSYS_IRATE, CSYS_NORMAL, 1 ,
1, "ktime", 7, CSYS_IRATE, CSYS_NORMAL, 1 ,
1, "stime", 8, CSYS_IRATE, CSYS_NORMAL, 1 ,
0, "incr", 9, CSYS_KRATE, CSYS_NORMAL, 1 ,
0, "rel", 10, CSYS_KRATE, CSYS_NORMAL, 1 ,
0, "tot", 11, CSYS_KRATE, CSYS_NORMAL, 1 ,
0, "x", 12, CSYS_ARATE, CSYS_NORMAL, 1 ,
0, "y", 13, CSYS_ARATE, CSYS_NORMAL, 1 ,
0, "init", 14, CSYS_ARATE, CSYS_NORMAL, 1 ,
0, "env", 15, CSYS_ARATE, CSYS_NORMAL, 1  };

csys_instrstruct csys_instr[CSYS_INSTRNUM] = {
0, "sine",0, CSYS_sine_VARNUM, &(csys_sine_vars[0]),1, 0 };

struct instr_line cm_sine[MAXCNOTES];
struct instr_line * cm_sine___first = &cm_sine[1];
struct instr_line * cm_sine___last = &cm_sine[0];
struct instr_line * cm_sine___end = &cm_sine[MAXCNOTES-1];
struct instr_line * cm_sine___next = NULL;

#define CSYS_CCPOS 0
#define CSYS_TOUCHPOS 128
#define CSYS_CHTOUCHPOS 256
#define CSYS_BENDPOS 257
#define CSYS_EXTPOS 258
#define CSYS_FRAMELEN 259
#define CSYS_MAXPRESETS 1

#define CSYS_NULLPROGRAM 0

struct instr_line **cmp_first[CSYS_MAXPRESETS] = {
&cm_sine___first};

struct instr_line **cmp_last[CSYS_MAXPRESETS] = {
&cm_sine___last};

struct instr_line **cmp_end[CSYS_MAXPRESETS] = {
&cm_sine___end};

struct instr_line **cmp_next[CSYS_MAXPRESETS] = {
&cm_sine___next};

#define CSYS_MAXEXTCHAN 16

struct instr_line **cme_first[CSYS_MAXEXTCHAN] = {
&cm_sine___first,&cm_sine___first,&cm_sine___first,&cm_sine___first,&cm_sine___first,
&cm_sine___first,&cm_sine___first,&cm_sine___first,&cm_sine___first,&cm_sine___first,
&cm_sine___first,&cm_sine___first,&cm_sine___first,&cm_sine___first,&cm_sine___first,
&cm_sine___first};

struct instr_line **cme_last[CSYS_MAXEXTCHAN] = {
&cm_sine___last,&cm_sine___last,&cm_sine___last,&cm_sine___last,&cm_sine___last,
&cm_sine___last,&cm_sine___last,&cm_sine___last,&cm_sine___last,&cm_sine___last,
&cm_sine___last,&cm_sine___last,&cm_sine___last,&cm_sine___last,&cm_sine___last,
&cm_sine___last};

struct instr_line **cme_end[CSYS_MAXEXTCHAN] = {
&cm_sine___end,&cm_sine___end,&cm_sine___end,&cm_sine___end,&cm_sine___end,
&cm_sine___end,&cm_sine___end,&cm_sine___end,&cm_sine___end,&cm_sine___end,
&cm_sine___end,&cm_sine___end,&cm_sine___end,&cm_sine___end,&cm_sine___end,
&cm_sine___end};

struct instr_line **cme_next[CSYS_MAXEXTCHAN] = {
&cm_sine___next,&cm_sine___next,&cm_sine___next,&cm_sine___next,&cm_sine___next,
&cm_sine___next,&cm_sine___next,&cm_sine___next,&cm_sine___next,&cm_sine___next,
&cm_sine___next,&cm_sine___next,&cm_sine___next,&cm_sine___next,&cm_sine___next,
&cm_sine___next};

int cme_preset[CSYS_MAXEXTCHAN] = {
0,CSYS_MAXPRESETS,CSYS_MAXPRESETS,CSYS_MAXPRESETS,CSYS_MAXPRESETS,
CSYS_MAXPRESETS,CSYS_MAXPRESETS,CSYS_MAXPRESETS,CSYS_MAXPRESETS,CSYS_MAXPRESETS,
CSYS_MAXPRESETS,CSYS_MAXPRESETS,CSYS_MAXPRESETS,CSYS_MAXPRESETS,CSYS_MAXPRESETS,
CSYS_MAXPRESETS};

int csys_bank = 0;
int csys_banklsb = 0;
int csys_bankmsb = 0;

#define MAXLINES 256
#define MAXSTATE 260


#define MAXVARSTATE 21
#define MAXTABLESTATE 1

/* ninstr: used for score, effects, */
/* and dynamic instruments          */

struct ninstr_types {

struct instr_line * iline; /* pointer to score line */
dstack v[MAXVARSTATE];     /* parameters & variables*/
struct tableinfo t[MAXTABLESTATE]; /* tables */

} ninstr[MAXSTATE];

#define CSYS_CDRIVER_ASCII
#define ASYS_OUTDRIVER_LINUX
#define ASYS_HASOUTPUT
#define ASYS_KSYNC


/*
#    Sfront, a SAOL to C translator    
#    This file: merged linux/freebsd audio driver for sfront
#    Copyright (C) 1999  Regents of the University of California
#    Copyright (C) 2000  Bertrand Petit
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License (Version 2) as
#    published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#    Maintainer: John Lazzaro, lazzaro@cs.berkeley.edu
*/

#define ASYSIO_LINUX   0
#define ASYSIO_FREEBSD 1

#define ASYSIO_OSTYPE  ASYSIO_LINUX

/****************************************************************/
/****************************************************************/
/*                linux audio driver for sfront                 */ 
/****************************************************************/

/**************************************************/
/* define flags for fifo mode, and for a timer to */
/* catch SAOL infinite loops                      */
/**************************************************/

#if (defined(ASYS_HASOUTPUT) && (ASYSIO_OSTYPE == ASYSIO_LINUX) && \
     defined(CSYS_CDRIVER_LINMIDI) && (ASYS_TIMEOPTION == ASYS_TIMESYNC) && \
     !defined(ASYS_HASINPUT))
#define ASYSIO_USEFIFO 1
#endif

#if (defined(ASYS_HASOUTPUT) && (ASYSIO_OSTYPE == ASYSIO_LINUX) && \
    (ASYS_TIMEOPTION != ASYS_TIMESYNC))
#define ASYSIO_USEFIFO 1
#endif

#ifndef ASYSIO_USEFIFO
#define ASYSIO_USEFIFO 0
#endif

/**************************************************/
/* include headers, based on flags set above      */
/**************************************************/

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#if (ASYSIO_OSTYPE == ASYSIO_LINUX)
#include <sys/soundcard.h>
#include <endian.h>
#endif

#if (ASYSIO_OSTYPE == ASYSIO_FREEBSD)
#include <machine/soundcard.h>
#include <machine/endian.h>
#endif

#include <signal.h>  
#include <sys/time.h>  

#if ASYSIO_USEFIFO
#include <sched.h>  
#if (ASYS_TIMEOPTION == ASYS_TIMESYNC)
#include <time.h>
#endif
#endif

/******************************/
/* other constant definitions */
/******************************/

#ifndef ASYSIO_DSPDEV
#define ASYSIO_DSPDEV "/dev/dsp"
#endif

/* determines native audio format */

#if (BYTE_ORDER == BIG_ENDIAN)
# define ASYSIO_AFORMAT AFMT_S16_BE
#else
# if (BYTE_ORDER == LITTLE_ENDIAN)
#  define ASYSIO_AFORMAT AFMT_S16_LE
# else
#  error "BYTE_ORDER not defined?"
# endif
#endif

/* codes for IO types */

#define ASYSIO_I  0
#define ASYSIO_O  1
#define ASYSIO_IO 2

/* minimum fragment size */

#define ASYSIO_FRAGMIN    16
#define ASYSIO_LOGFRAGMIN 4

/* number of silence buffers */

#define ASYSO_LNUMBUFF 4

/* maximum number of I/O retries before termination */

#define ASYSIO_MAXRETRY 256

#if ASYSIO_USEFIFO                      /* SCHED_FIFO constants for ksync()  */

#if (ASYS_TIMEOPTION == ASYS_TIMESYNC)
#define ASYSIO_SYNC_TIMEOUT    5        /* idle time to leave SCHED_FIFO     */
#define ASYSIO_SYNC_ACTIVE     0        /* machine states for noteon timeout */
#define ASYSIO_SYNC_WAITING    1
#define ASYSIO_SYNC_SCHEDOTHER 2
#else
#define ASYSIO_MAXBLOCK ((int)KRATE)*2  /* max wait tor let SCHED_OTHERs run */
#endif

#endif

/************************/
/* variable definitions */
/************************/

int  asysio_fd;                    /* device pointer */
long asysio_srate;                 /* sampling rate */
long asysio_channels;              /* number of channels */
long asysio_size;                  /* # samples in a buffer */
long asysio_bsize;                 /* actual # bytes in a buffer */            
long asysio_requested_bsize;       /* requested # bytes in a buffer */        
long asysio_input;                 /* 1 if ASYSIO */
long asysio_blocktime;             /* time (in bytes) blocked in kcycle */

struct count_info asysio_ptr;      /* for GET{I,O}*PTR  ioctl calls */
struct audio_buf_info asysio_info; /* for GET{I,O}SPACE ioctl calls */

#if defined(ASYS_HASOUTPUT)
short * asyso_buf = NULL;          /* output buffer */
int asysio_puts;                   /* total number of putbufs */
int asysio_reset;                  /* flags an overrun */
#endif

#if defined(ASYS_HASINPUT)
short * asysi_buf = NULL;          /* input buffer */
struct audio_buf_info asysi_info;  /* input dma status */
#endif

sigset_t asysio_iloop_mask;            /* for masking off iloop interrupt */
struct sigaction asysio_iloop_action;  /* for setting up iloop interrupt  */
struct itimerval asysio_iloop_timer;   /* for setting up iloop timer      */

#if ASYSIO_USEFIFO 
int asysio_fifo;                       /* can get into sched_fifo mode */
struct sched_param asysio_fifoparam;   /* param block for fifo mode */
struct sched_param asysio_otherparam;  /* param block for other mode */

#if (ASYS_TIMEOPTION == ASYS_TIMESYNC)

/* state machine variables for noteon timeout */
int    asysio_sync_state;
time_t asysio_sync_waitstart;
extern int csysi_newnote;       /* from linmidi */

#else

/* state to detect long periods w/o blocking */
int asysio_sync_noblock;                /* how many acycles since last block */
struct timespec asysio_sync_sleeptime;  /* time to wait during forced block  */

#endif

#endif

#if defined(ASYS_KSYNC)                      /* ksync() state */
struct count_info asysio_sync_ptr;           
int asysio_sync_target, asysio_sync_incr;    
float asysio_sync_cpuscale;                  
#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                      shutdown routines                       */
/*______________________________________________________________*/


/****************************************************************/
/*                    shuts down soundcard                      */
/****************************************************************/

void asysio_shutdown(void)

{
  if (ioctl(asysio_fd, SNDCTL_DSP_SYNC, 0) == -1)
    {
      fprintf(stderr, "\nSoundcard Error: SNDCTL_DSP_SYNC Ioctl Problem\n");
      fprintf(stderr, "Errno Message: %s\n\n", strerror(errno));
    }
  close(asysio_fd);

  /* so that a slow exit doesn't trigger timer */

  asysio_iloop_timer.it_value.tv_sec = 0;
  asysio_iloop_timer.it_value.tv_usec = 0;
  asysio_iloop_timer.it_interval.tv_sec = 0;
  asysio_iloop_timer.it_interval.tv_usec = 0;
  
  if (setitimer(ITIMER_PROF, &asysio_iloop_timer, NULL) < 0)
    {
      fprintf(stderr, "\nSoundcard Driver Error:\n\n");
      fprintf(stderr, "  Couldn't set up ITIMER_PROF timer.\n");
      fprintf(stderr, "  Errno Message: %s\n\n", strerror(errno));
    }
}


#if (defined(ASYS_HASOUTPUT)&&(!defined(ASYS_HASINPUT)))

/****************************************************************/
/*                    shuts down audio output                   */
/****************************************************************/

void asys_oshutdown(void)

{
  asysio_shutdown();
  if (asyso_buf != NULL)
    free(asyso_buf);
}

#endif

#if (!defined(ASYS_HASOUTPUT)&&(defined(ASYS_HASINPUT)))

/****************************************************************/
/*                    shuts down audio input                    */
/****************************************************************/

void asys_ishutdown(void)

{
  asysio_shutdown();  
  if (asysi_buf != NULL)
    free(asysi_buf);
}

#endif


#if (defined(ASYS_HASOUTPUT)&&(defined(ASYS_HASINPUT)))

/****************************************************************/
/*              shuts down audio input and output               */
/****************************************************************/

void asys_ioshutdown(void)

{
  asysio_shutdown();
  if (asyso_buf != NULL)
    free(asyso_buf);
  if (asysi_buf != NULL)
    free(asysi_buf);
}

#endif


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                 initialization routines                      */
/*______________________________________________________________*/


/****************************************************************/
/*          generic error-checking ioctl wrapper                */
/****************************************************************/

#define ASYSIO_IOCTL_CALL(x,y,z)  do { if (ioctl(x,y,z) == -1){\
      fprintf(stderr, "  Error: %s Ioctl Problem\n", #y ); \
      fprintf(stderr, "  Errno Message: %s\n\n", strerror(errno));\
      close(asysio_fd); return ASYS_ERROR;}} while (0)

#define  ASYSIO_ERROR_RETURN(x) do {\
      fprintf(stderr, "  Error: %s.\n", x);\
      fprintf(stderr, "  Errno Message: %s\n\n", strerror(errno));\
      close(asysio_fd);\
      return ASYS_ERROR; } while (0)

#define  ASYSIO_ERROR_RETURN_NOERRNO(x) do {\
      fprintf(stderr, "  Error: %s.\n", x);\
      close(asysio_fd);\
      return ASYS_ERROR; } while (0)


/****************************************************************/
/*               opens the soudcard device                      */
/****************************************************************/

int asysio_opendevice(int dir, int toption)

{

  switch(dir) {
  case ASYSIO_I:
    asysio_fd = open(ASYSIO_DSPDEV, O_RDONLY, 0);
    asysio_input = 1;
    break;
  case ASYSIO_O:
    asysio_fd = open(ASYSIO_DSPDEV, O_WRONLY, 0);
    asysio_input = 0;
    break;
  case ASYSIO_IO:
    asysio_fd = open(ASYSIO_DSPDEV, O_RDWR, 0);
    asysio_input = 1;
    break;
  default:
    fprintf(stderr, "  Error: Unexpected dir parameter value in \n");
    fprintf(stderr, "         asysio_setup.\n\n");
    return ASYS_ERROR;
  }

  if (asysio_fd == -1)
    {
      fprintf(stderr, "  Error: Can't open device %s (%s)\n\n", ASYSIO_DSPDEV,
	      strerror(errno));
      return ASYS_ERROR;
    }
  return ASYS_DONE;

}

/****************************************************************/
/*         signal handler for trapping SAOL infinite loops      */
/****************************************************************/

void asysio_iloop_handler(int signum) 
{   
  fprintf(stderr, "  Error: Either\n\n");
  fprintf(stderr, "    [1] The SAOL program has an infinite loop in it, or\n");
  fprintf(stderr, "    [2] Content is too complex for real-time work.\n\n");
  fprintf(stderr, "  Exiting program ...\n\n");
  exit(0);
}


/****************************************************************/
/*         initializes iloop (heartbeat) interrupt              */
/****************************************************************/

int asysio_initiloop(void)

{

  /*********************************************************/
  /* set up signal handler for infinite-loop (iloop) timer */
  /*********************************************************/
  
  if (sigemptyset(&asysio_iloop_action.sa_mask) < 0)
    ASYSIO_ERROR_RETURN("Couldn't run sigemptyset(iloop) OS call");

  /* infinite-loop timer wins over midi overrun timer */

  if (sigaddset(&asysio_iloop_action.sa_mask, SIGALRM) < 0)
    ASYSIO_ERROR_RETURN("Couldn't run sigaddset(iloop) OS call");

  asysio_iloop_action.sa_flags = SA_RESTART;
  asysio_iloop_action.sa_handler = asysio_iloop_handler;
  
  if (sigaction(SIGPROF, &asysio_iloop_action, NULL) < 0)
    ASYSIO_ERROR_RETURN("Couldn't set up SIGPROF signal handler");


  /************************/
  /* set up timer and arm */
  /************************/

  asysio_iloop_timer.it_value.tv_sec = 3;
  asysio_iloop_timer.it_value.tv_usec = 0;
  asysio_iloop_timer.it_interval.tv_sec = 0;
  asysio_iloop_timer.it_interval.tv_usec = 0;

  if (setitimer(ITIMER_PROF, &asysio_iloop_timer, NULL) < 0)
    ASYSIO_ERROR_RETURN("Couldn't set up ITIMER_PROF timer");

  return ASYS_DONE;
}

/****************************************************************/
/*                 initializes sched_fifo                       */
/****************************************************************/

int asysio_initscheduler(void)

{

#if ASYSIO_USEFIFO

  /*******************************/
  /* set up sched_fifo variables */
  /*******************************/

  memset(&asysio_otherparam, 0, sizeof(struct sched_param));
  memset(&asysio_fifoparam, 0, sizeof(struct sched_param));
 
  if ((asysio_fifoparam.sched_priority =
       sched_get_priority_max(SCHED_FIFO)) < 0)
    ASYSIO_ERROR_RETURN("Couldn't get scheduling priority");

  asysio_fifoparam.sched_priority--;

  /********************************/
  /* try to enter sched-fifo mode */
  /********************************/

  asysio_fifo = !sched_setscheduler(0, SCHED_FIFO, &asysio_fifoparam);

#endif

  return ASYS_DONE;
}

/****************************************************************/
/*                 prints startup screen                        */
/****************************************************************/

int asysio_screenwriter(void)

{
  int i, found;
  int haslinmidi = 0;
  float actual_latency;


  fprintf(stderr, "%s ",  (ASYS_LATENCYTYPE == ASYS_HIGHLATENCY)? 
	  "Streaming" : "Interactive");

  fprintf(stderr, "%s Audio ", (asysio_channels == 2) ? "Stereo" : "Mono");

#if defined(ASYS_HASOUTPUT)
  fprintf(stderr, "%s", asysio_input ? "Input/Output" : "Output");
#else
  fprintf(stderr, "Input");
#endif

  found = i = 0;
  while (i < csys_sfront_argc)
    {
      if (!(strcmp(csys_sfront_argv[i],"-bitc") && 
	    strcmp(csys_sfront_argv[i],"-bit") &&
	    strcmp(csys_sfront_argv[i],"-orc")))
	{
	  i++;
	  fprintf(stderr, " for %s", csys_sfront_argv[i]);
	  found = 1;
	  break;
	}
      i++;
    }
  if (!found)
    fprintf(stderr, " for UNKNOWN");


  i = 0;
  while (i < csys_sfront_argc)
    {
      if (!strcmp(csys_sfront_argv[i],"-cin"))
	{
	  i++;
	  fprintf(stderr, " (-cin %s)", csys_sfront_argv[i]);
	  break;
	}
      i++;
    }
  fprintf(stderr, "\n\n");


#if defined(CSYS_CDRIVER_LINMIDI)

  haslinmidi = 1;

#endif

#if (defined(CSYS_CDRIVER_LINMIDI) || defined(CSYS_CDRIVER_ALSAMIDI)\
      || defined(CSYS_CDRIVER_ALSASEQ) || defined(CSYS_CDRIVER_ASCII))

  /* list midi presets available */

  fprintf(stderr, 
	  "MIDI Preset Numbers (use MIDI controller to select):\n\n");

  for (i = 0; i < CSYS_PRESETNUM; i++)
    {
      fprintf(stderr, "%3i. %s", 
	      csys_presets[i].preset,
	      csys_instr[csys_presets[i].index].name);
      if ((i&1))
	fprintf(stderr, "\n");
      else
	{
	  fprintf(stderr, "\t\t");
	  if (i == (CSYS_PRESETNUM-1))
	    fprintf(stderr, "\n");
	}
    }
  fprintf(stderr, "\n");

#endif

#if defined(CSYS_CDRIVER_ASCII)

  fprintf(stderr, 
  "To play tunes on ASCII keyboard: a-z for notes, 0-9 for MIDI presets,\n");
  fprintf(stderr, 
  "cntrl-C exits. If autorepeat interferes, exit and run 'xset -r' (in X).\n\n");
  
#endif

  /* diagnose best flags to use, and if they are used */

#ifdef ASYS_HASOUTPUT

  if ((ASYS_LATENCYTYPE == ASYS_HIGHLATENCY) || asysio_input ||
      (!haslinmidi))
    {
      if (geteuid() || (ASYS_TIMEOPTION == ASYS_TIMESYNC))
	{
	  fprintf(stderr, "For best results, make these changes:\n"); 
	  fprintf(stderr, "\n");
	  if (ASYS_TIMEOPTION == ASYS_TIMESYNC)
	    fprintf(stderr, "   * Remove sfront -timesync flag\n");
	  if (geteuid())
	    fprintf(stderr, "   * Run sa.c executable as root.\n");
	  fprintf(stderr, "\n");
	}
    }
  else
    {
      fprintf(stderr, "This application runs best as root (%s), with:\n",
	      !geteuid() ? "which you are": "which you aren't"); 
      fprintf(stderr, "\n");
      fprintf(stderr, "  [1] Sfront -playback flag. Good audio quality, keeps\n");
      fprintf(stderr, "      the mouse/kbd alive");
      fprintf(stderr, "%s.\n", (ASYS_TIMEOPTION == ASYS_PLAYBACK) ?
	      " (currently chosen)":"");
      fprintf(stderr, "  [2] Sfront -timesync flag. Better quality, console\n");
      fprintf(stderr, "      freezes during MIDI input");
      fprintf(stderr, "%s.\n", (ASYS_TIMEOPTION == ASYS_TIMESYNC) ?
	      " (currently chosen)":"");
      fprintf(stderr, "\n");
    }

#endif

  /* latency information */

#if (defined(ASYS_HASOUTPUT))

  ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETOSPACE, &asysio_info);

  fprintf(stderr, "If audio artifacts still occur, try");
  
  actual_latency = ATIME*ASYSO_LNUMBUFF*(asysio_size >> (asysio_channels - 1));
  
  if (asysio_info.fragstotal < ASYSO_LNUMBUFF)
    {
      fprintf(stderr, " sfront -latency %f flag, and see\n", 
	      0.5F*actual_latency);
    }
  else
    {
      fprintf(stderr, " sfront -latency %f flag, and see\n", 
	      2.0F*actual_latency);

    }
  if (ASYS_LATENCYTYPE == ASYS_LOWLATENCY)
    fprintf(stderr, "http://www.cs.berkeley.edu/"
	    "~lazzaro/sa/sfman/user/use/index.html#interact\n") ;
  else
    fprintf(stderr, "http://www.cs.berkeley.edu/"
	    "~lazzaro/sa/sfman/user/use/index.html#stream\n") ;

  fprintf(stderr, "\n");

  if ((asysio_bsize != ASYSIO_FRAGMIN) &&
      (asysio_bsize == asysio_requested_bsize) && 
      (ASYS_LATENCYTYPE == ASYS_LOWLATENCY))
  {
    fprintf(stderr, "If interactive response is slow, try ");
    fprintf(stderr, "sfront -latency %f flag.\n", 0.5F*actual_latency);
    fprintf(stderr, "\n");
  }

#endif

  fprintf(stderr, 
  "USE AT YOUR OWN RISK. Running as root may damage your file system,\n");
  fprintf(stderr, 
  "and network use may result in a malicious attack on your machine.\n\n");

#if (ASYSIO_USEFIFO && (ASYS_TIMEOPTION == ASYS_TIMESYNC))

  if (!geteuid())
    {
      fprintf(stderr, 
      "NOTE: Mouse and keyboard are frozen for %i seconds after a MIDI\n",
	      ASYSIO_SYNC_TIMEOUT);
      fprintf(stderr, 
	      "NoteOn or NoteOff is received. Do not be alarmed.\n");
    }

#endif

  if (NSYS_NET)
    fprintf(stderr, "Network status: Contacting SIP server\n");

  return ASYS_DONE;

}


/****************************************************************/
/*        setup operations common to input and output           */
/****************************************************************/

int asysio_setup(long srate, long channels, int dir, int toption)

{
  long i, j, maxfrag;

  /******************/
  /* open soundcard */
  /******************/

  if (asysio_opendevice(dir, toption) == ASYS_ERROR)
    return ASYS_ERROR;
  
  /**************************************/
  /* set up bidirectional I/O if needed */
  /**************************************/

  if (dir == ASYSIO_IO)
    {

#if (ASYSIO_OSTYPE != ASYSIO_FREEBSD)

      ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_SETDUPLEX, 0);

#endif 

      ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETCAPS, &j);

      if (!(j & DSP_CAP_DUPLEX))
	ASYSIO_ERROR_RETURN_NOERRNO("Sound card can't do bidirectional audio");
    }

  /************************/
  /* range check channels */
  /************************/

  if (channels > 2)
    ASYSIO_ERROR_RETURN_NOERRNO("Sound card can't handle > 2 channels");

  /*********************/
  /* set fragment size */
  /*********************/

  j = ASYSIO_LOGFRAGMIN;
  i = ASYSIO_FRAGMIN >> channels;   /* only works for channels = 1, 2 */

  /* find closest power-of-two fragment size to latency request */

  while (2*ATIME*i*ASYSO_LNUMBUFF < ASYS_LATENCY)
    {
      i <<= 1;
      j++;
    }
  if ((ATIME*2*i*ASYSO_LNUMBUFF - ASYS_LATENCY) < 
      (ASYS_LATENCY - ATIME*i*ASYSO_LNUMBUFF))
    {
      i <<= 1;
      j++;
    }

  asysio_requested_bsize = 2*i*channels;

  maxfrag = (ASYS_TIMEOPTION != ASYS_TIMESYNC) ? ASYSO_LNUMBUFF :
             ASYSO_LNUMBUFF + ((ACYCLE/i) + 1);

  j |= (maxfrag << 16);
  ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_SETFRAGMENT, &j);

  /********************/
  /* set audio format */
  /********************/

  j = ASYSIO_AFORMAT;
  ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_SETFMT, &j);

  if (j != ASYSIO_AFORMAT)
    ASYSIO_ERROR_RETURN_NOERRNO("Soundcard can't handle native shorts");

  /****************************************************/
  /* set number of channels -- later add channels > 2 */
  /****************************************************/

  asysio_channels = channels--;
  ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_STEREO, &channels);

  if (channels != (asysio_channels-1))
    ASYSIO_ERROR_RETURN_NOERRNO("Soundcard can't handle number of channels");

  /*********************/
  /* set sampling rate */
  /*********************/

  asysio_srate = srate;
  ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_SPEED, &srate);

  if (abs(asysio_srate - srate) > 1000)
    ASYSIO_ERROR_RETURN_NOERRNO("Soundcard can't handle sampling rate");

  /******************************/
  /* compute actual buffer size */
  /******************************/

  ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETBLKSIZE, &asysio_bsize);
  asysio_size = asysio_bsize >> 1;

  /*************************/
  /* print out info screen */
  /*************************/

  if (asysio_screenwriter() == ASYS_ERROR)
    return ASYS_ERROR;

  /*********************************/
  /* set SCHED_FIFO if appropriate */
  /*********************************/

  if (asysio_initscheduler() == ASYS_ERROR)
    return ASYS_ERROR;

  /**********************************/
  /* set up iloop (heartbeat) timer */
  /**********************************/

  if (asysio_initiloop() == ASYS_ERROR)
    return ASYS_ERROR;

  return ASYS_DONE;
}



#if (defined(ASYS_HASOUTPUT) && !defined(ASYS_HASINPUT))

/****************************************************************/
/*        sets up audio output for a given srate/channels       */
/****************************************************************/

int asys_osetup(long srate, long ochannels, long osample, 
                char * oname, long toption)

{
  if (asysio_setup(srate, ochannels, ASYSIO_O, toption) == ASYS_ERROR)
    return ASYS_ERROR;

  if (!(asyso_buf = (short *)calloc(asysio_size, sizeof(short))))
    ASYSIO_ERROR_RETURN("Can't allocate output buffer");

  return ASYS_DONE;
}

#endif


#if (!defined(ASYS_HASOUTPUT) && defined(ASYS_HASINPUT))

/****************************************************************/
/*        sets up audio input for a given srate/channels       */
/****************************************************************/

int asys_isetup(long srate, long ichannels, long isample, 
                char * iname, long toption)

{
  if (asysio_setup(srate, ichannels, ASYSIO_I, toption) == ASYS_ERROR)
    return ASYS_ERROR;
  if (!(asysi_buf = (short *)malloc(asysio_bsize)))
    ASYSIO_ERROR_RETURN("Can't allocate input buffer");

  return ASYS_DONE;
}

#endif


#if (defined(ASYS_HASOUTPUT) && defined(ASYS_HASINPUT))

/****************************************************************/
/*   sets up audio input and output for a given srate/channels  */
/****************************************************************/

int asys_iosetup(long srate, long ichannels, long ochannels,
                 long isample, long osample, 
                 char * iname, char * oname, long toption)


{

  if (ichannels != ochannels)
    ASYSIO_ERROR_RETURN_NOERRNO
      ("Soundcard needs SAOL inchannels == outchannels");

  if (asysio_setup(srate, ichannels, ASYSIO_IO, toption) == ASYS_ERROR)
    return ASYS_ERROR;

  if (!(asysi_buf = (short *)malloc(asysio_bsize)))
    ASYSIO_ERROR_RETURN("Can't allocate input buffer");

  if (!(asyso_buf = (short *)calloc(asysio_size, sizeof(short))))
    ASYSIO_ERROR_RETURN("Can't allocate output buffer");

  return ASYS_DONE;
}

#endif


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*            input, output, and recovery routines              */
/*______________________________________________________________*/


#if defined(ASYS_HASINPUT)

/****************************************************************/
/*               gets one frame of audio from input             */
/****************************************************************/

int asys_getbuf(ASYS_ITYPE * asys_ibuf[], long * isize)

{
  long diffcompute, starttime;
  long size, recv, bptr, retry;

  *isize = asysio_size;

  if (*asys_ibuf == NULL)
    *asys_ibuf = asysi_buf;

  ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETISPACE, &asysio_info);

#if defined(ASYS_HASOUTPUT)

  if (diffcompute = (asysio_info.bytes < asysio_bsize))
    {  
      ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETOPTR, &asysio_ptr);
      starttime = asysio_ptr.bytes;
    }

#endif

  retry = bptr = 0;
  size = asysio_bsize;

  while ((recv = read(asysio_fd, &((*asys_ibuf)[bptr]), size)) != size)
    {      
      if (++retry > ASYSIO_MAXRETRY)
	ASYSIO_ERROR_RETURN("Too many I/O retries -- asys_getbuf");

      if (recv < 0)  /* errors */
	{
	  if ((errno == EAGAIN) || (errno == EINTR))
	    continue;   
	  else
	    ASYSIO_ERROR_RETURN("Read error on output audio device");
	}
      else
	{
	  bptr += recv; /* partial read */
	  size -= recv;
	}
    }

#if defined(ASYS_HASOUTPUT)

  if (diffcompute)
    {  
      ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETOPTR, &asysio_ptr);
      asysio_blocktime += (asysio_ptr.bytes - starttime);
    }

#endif

  return ASYS_DONE;
}

#endif


#if defined(ASYS_HASOUTPUT)

/****************************************************************/
/*               sends one frame of audio to output             */
/****************************************************************/

int asys_putbuf(ASYS_OTYPE * asys_obuf[], long * osize)


{
  long size, sent, bptr, retry;
  long diffcompute, starttime;

  size = (*osize)*2;


  if (asysio_reset)
    return ASYS_DONE;

  ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETOSPACE, &asysio_info);

  asysio_reset = (++asysio_puts > ASYSO_LNUMBUFF) && 
    (asysio_info.fragments == asysio_info.fragstotal);
  if (asysio_reset)
    return ASYS_DONE;

#if (ASYS_TIMEOPTION != ASYS_TIMESYNC)

  if (diffcompute = (asysio_info.bytes < size))
    {  
      ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETOPTR, &asysio_ptr);
      starttime = asysio_ptr.bytes;
    }

#endif

  retry = bptr = 0;
  while ((sent = write(asysio_fd, &((*asys_obuf)[bptr]), size)) != size)
    {
      if (++retry > ASYSIO_MAXRETRY)
	ASYSIO_ERROR_RETURN("Too many I/O retries -- asys_putbuf");

      if (sent < 0)  /* errors */
	{
	  if ((errno == EAGAIN) || (errno == EINTR))
	    continue;   
	  else
	    ASYSIO_ERROR_RETURN("Write error on output audio device");
	}
      else
	{
	  bptr += sent;  /* partial write */
	  size -= sent;
	}
    }

#if (ASYS_TIMEOPTION != ASYS_TIMESYNC)

  if (diffcompute)
    {  
      ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETOPTR, &asysio_ptr);
      asysio_blocktime += (asysio_ptr.bytes - starttime);
    }

#endif

  *osize = asysio_size;
  return ASYS_DONE;
}


/****************************************************************/
/*        creates buffer, and generates starting silence        */
/****************************************************************/

int asys_preamble(ASYS_OTYPE * asys_obuf[], long * osize)

{
  int i;

  *asys_obuf = asyso_buf;
  *osize = asysio_size;

  for(i = 0; i < ASYSO_LNUMBUFF; i++)
    if (asys_putbuf(asys_obuf, osize) == ASYS_ERROR)
      return ASYS_ERROR;

  return ASYS_DONE;
}


/****************************************************************/
/*               recovers from an overrun                       */
/****************************************************************/

int asysio_recover(void)

{
  long size, recv, bptr, retry;
  int i;

  asysio_reset = 0;

  memset(asyso_buf, 0, asysio_bsize);

  /*************************/
  /* flush input if needed */
  /*************************/

#if defined(ASYS_HASINPUT)

  ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETISPACE, &asysi_info);
  
  while (asysi_info.fragments > 0)
    {
      retry = bptr = 0;
      size = asysio_bsize;

      while ((recv = read(asysio_fd, &(asysi_buf[bptr]), size)) != size)
	{      
	  if (++retry > ASYSIO_MAXRETRY)
	    ASYSIO_ERROR_RETURN("Too many I/O retries -- asysio_recover");

	  if (recv < 0)  /* errors */
	    {
	      if ((errno == EAGAIN) || (errno == EINTR))
		continue;   
	      else
		ASYSIO_ERROR_RETURN("Read error on output audio device");
	    }
	  else
	    {
	      bptr += recv; /* partial read */
	      size -= recv;
	    }
	}

      ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETISPACE, &asysi_info);
    }

  ibusidx = 0;
  if (asys_getbuf(&asys_ibuf, &asys_isize)==ASYS_ERROR)
    return ASYS_ERROR;

#endif

  /**************************************/
  /* fill latency interval with silence */ 
  /**************************************/

  asysio_puts = 0;
  for(i = 0; i < ASYSO_LNUMBUFF; i++)
    if (asys_putbuf(&asyso_buf, &asysio_size) == ASYS_ERROR)
      return ASYS_ERROR;

  return ASYS_DONE;

}


#endif


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*        ksync() system for time synchronization          */ 
/*_________________________________________________________*/

#if defined(ASYS_KSYNC)

/***********************************************************/
/*         initializes k-rate boundaries sync              */
/***********************************************************/

void ksyncinit()

{
  asysio_sync_target = asysio_sync_incr = ACYCLE*asysio_channels*2;  
  asysio_sync_cpuscale = 1.0F/asysio_sync_incr;

  /* for -timesync, set up SCHED_FIFO watchdog state machine */

#if (ASYSIO_USEFIFO && (ASYS_TIMEOPTION == ASYS_TIMESYNC))

  if (asysio_fifo)

    {
      asysio_sync_state = ASYSIO_SYNC_SCHEDOTHER;
      if (sched_setscheduler(0, SCHED_OTHER, &asysio_otherparam))
	epr(0,NULL,NULL,"internal error -- sched_other unavailable");
    }

#endif

  /* elsewise, set up SCHED_FIFO monitor to force blocking */

#if (ASYSIO_USEFIFO && (ASYS_TIMEOPTION != ASYS_TIMESYNC))

  asysio_sync_noblock = 0;
  asysio_sync_sleeptime.tv_sec = 0;
  asysio_sync_sleeptime.tv_nsec = 2000001;  /* 2ms + epsilon forces block */
 
#endif

}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*     different ksync()s for -timesync and -playback       */ 
/*__________________________________________________________*/


#if (ASYS_TIMEOPTION != ASYS_TIMESYNC)

/***********************************************************/
/*         synchronizes on k-rate boundaries               */
/***********************************************************/

float ksync()

{
  float ret;
  long comptime;

  if (asysio_reset)
    {
      if (asysio_recover()==ASYS_ERROR)
	epr(0,NULL,NULL, "Soundcard error -- failed recovery.");
      asysio_sync_target = asysio_sync_incr;
      ret = 1.0F;
    }
  else
    {

      ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETOPTR, &asysio_sync_ptr);

      if (asysio_sync_target == asysio_sync_incr)
	ret = 0.0F;
      else
	{
	  comptime = asysio_sync_ptr.bytes - asysio_blocktime;
	  if (comptime > asysio_sync_target)
	    ret = 1.0F;
	  else
	    ret = (asysio_sync_cpuscale*
		   (asysio_sync_incr - (asysio_sync_target - comptime)));
	}
      
      if ((asysio_sync_target = asysio_sync_incr + asysio_sync_ptr.bytes) < 0)
	epr(0,NULL,NULL,"Soundcard error -- rollover.");
    }

  /* reset infinite-loop timer */

  if (setitimer(ITIMER_PROF, &asysio_iloop_timer, NULL) < 0)
    {
      fprintf(stderr, "  Runtime Errno Message: %s\n", strerror(errno));
      epr(0,NULL,NULL, "Soundcard error -- Couldn't reset ITIMER_PROF");
    }

#if ASYSIO_USEFIFO

  if (asysio_fifo)
    {
      /* let other processes run if pending too long */

      if (asysio_blocktime)
	asysio_sync_noblock = 0;
      else
	asysio_sync_noblock++;

      if (asysio_sync_noblock > ASYSIO_MAXBLOCK)
	{
	  nanosleep(&asysio_sync_sleeptime, NULL); 
	  asysio_sync_noblock = 0;
	}
    }

#endif

  asysio_blocktime = 0;
  return ret;
}

#endif


#if (ASYS_TIMEOPTION == ASYS_TIMESYNC)

/***********************************************************/
/*         synchronizes on k-rate boundaries               */
/***********************************************************/

float ksync()

{
  float ret;
  long comptime;

  if (!asysio_reset)
    {
      ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETOPTR, &asysio_sync_ptr);
      if (asysio_sync_ptr.bytes > asysio_sync_target)
	{
	  comptime = asysio_sync_ptr.bytes - asysio_blocktime;
	  if (comptime < asysio_sync_target)
	    ret = (asysio_sync_cpuscale*
		   (asysio_sync_incr - (asysio_sync_target - comptime)));
	  else
	    ret = 1.0F;
	  ret = (asysio_sync_target != asysio_sync_incr) ? ret : 0.0F;
	}
      else
	{	  
	  comptime = asysio_sync_ptr.bytes - asysio_blocktime;
	  ret = (asysio_sync_cpuscale*
		 (asysio_sync_incr - (asysio_sync_target - comptime)));
	  asysio_reset = asysio_input && 
	    ((asysio_sync_target-asysio_sync_ptr.bytes) == asysio_sync_incr);
	  while ((asysio_sync_ptr.bytes < asysio_sync_target) && !asysio_reset)
	    {  
	      ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETOSPACE, &asysio_info);
	      asysio_reset = (asysio_info.fragments == asysio_info.fragstotal);
	      ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETOPTR,&asysio_sync_ptr);
	    }
	}
    }
  if (asysio_reset)
    {
      if (asysio_recover()==ASYS_ERROR)
	epr(0,NULL,NULL,"Sound driver error -- failed recovery.");
      ASYSIO_IOCTL_CALL(asysio_fd, SNDCTL_DSP_GETOPTR, &asysio_sync_ptr);
      asysio_sync_target = asysio_sync_ptr.bytes;
      ret = 1.0F;
    }
  if ((asysio_sync_target += asysio_sync_incr) < 0)
    epr(0,NULL,NULL,"Sound driver error -- rollover.");

  /* reset infinite-loop timer */

  if (setitimer(ITIMER_PROF, &asysio_iloop_timer, NULL) < 0)
    {
      fprintf(stderr, "  Runtime Errno Message: %s\n", strerror(errno));
      epr(0,NULL,NULL, "Soundcard error -- Couldn't reset ITIMER_PROF");
    }

#if ASYSIO_USEFIFO

  if (asysio_fifo)
    {
      switch (asysio_sync_state) {
      case ASYSIO_SYNC_ACTIVE:
	if (!csysi_newnote)
	  {
	    asysio_sync_state = ASYSIO_SYNC_WAITING;
	    asysio_sync_waitstart = time(NULL);
	  }
	break;
      case ASYSIO_SYNC_WAITING:
	if (csysi_newnote)
	  asysio_sync_state = ASYSIO_SYNC_ACTIVE;
	else
	  if ((time(NULL) - asysio_sync_waitstart) >= ASYSIO_SYNC_TIMEOUT)
	    {
	      asysio_sync_state = ASYSIO_SYNC_SCHEDOTHER;
	      if (sched_setscheduler(0, SCHED_OTHER, &asysio_otherparam))
		epr(0,NULL,NULL,"internal error -- sched_other unavailable");
	    }
	break;
      case ASYSIO_SYNC_SCHEDOTHER:
	if (csysi_newnote)
	  {
	    asysio_sync_state = ASYSIO_SYNC_ACTIVE;
	    if (sched_setscheduler(0, SCHED_FIFO, &asysio_fifoparam))
	      fprintf(stderr, "  Note: Process no longer root, " 
		      "improved audio quality no longer possible.\n");
	  }
	break;
      }
    }

#endif

  asysio_blocktime = 0;
  return ret;
}

#endif

#endif /* ASYS_KSYNC */

#undef ASYSIO_IOCTL_CALL
#undef ASYSIO_ERROR_RETURN
#undef ASYSIO_ERROR_RETURN_NOERRNO
#undef ASYSIO_LINUX
#undef ASYSIO_FREEBSD
#undef ASYSIO_OSTYPE
#undef ASYSIO_DSPDEV
#undef ASYSIO_AFORMAT
#undef ASYSIO_I  
#undef ASYSIO_O  
#undef ASYSIO_IO 
#undef ASYSIO_FRAGMIN
#undef ASYSIO_LOGFRAGMIN 
#undef ASYSO_LNUMBUFF
#undef ASYSIO_MAXRETRY

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                    end of soundcard driver                   */
/*______________________________________________________________*/


#undef ASYS_HASOUTPUT
#undef ASYS_KSYNC


#define CSYS_DONE 0
#define CSYS_ERROR 1

#define CSYS_NONE 0
#define CSYS_MIDIEVENTS 1
#define CSYS_EVENTS 3

#define CSYS_MIDI_NUMCHAN  16

#define CSYS_MIDI_SPECIAL  0x70
#define CSYS_MIDI_NOOP     0x70
#define CSYS_MIDI_NEWTEMPO 0x71
#define CSYS_MIDI_ENDTIME  0x72
#define CSYS_MIDI_POWERUP  0x73
#define CSYS_MIDI_NOTEOFF  0x80
#define CSYS_MIDI_NOTEON   0x90
#define CSYS_MIDI_PTOUCH   0xA0
#define CSYS_MIDI_CC       0xB0
#define CSYS_MIDI_PROGRAM  0xC0
#define CSYS_MIDI_CTOUCH   0xD0
#define CSYS_MIDI_WHEEL    0xE0
#define CSYS_MIDI_SYSTEM   0xF0

#define CSYS_MIDI_SYSTEM_SYSEX_START  0xF0
#define CSYS_MIDI_SYSTEM_QFRAME       0xF1
#define CSYS_MIDI_SYSTEM_SONG_PP      0xF2
#define CSYS_MIDI_SYSTEM_SONG_SELECT  0xF3
#define CSYS_MIDI_SYSTEM_UNUSED1      0xF4
#define CSYS_MIDI_SYSTEM_UNUSED2      0xF5
#define CSYS_MIDI_SYSTEM_TUNE_REQUEST 0xF6
#define CSYS_MIDI_SYSTEM_SYSEX_END    0xF7
#define CSYS_MIDI_SYSTEM_CLOCK        0xF8
#define CSYS_MIDI_SYSTEM_TICK         0xF9
#define CSYS_MIDI_SYSTEM_START        0xFA
#define CSYS_MIDI_SYSTEM_CONTINUE     0xFB
#define CSYS_MIDI_SYSTEM_STOP         0xFC
#define CSYS_MIDI_SYSTEM_UNUSED3      0xFD
#define CSYS_MIDI_SYSTEM_SENSE        0xFE
#define CSYS_MIDI_SYSTEM_RESET        0xFF

#define CSYS_MIDI_CC_BANKSELECT_MSB  0x00
#define CSYS_MIDI_CC_MODWHEEL_MSB    0x01
#define CSYS_MIDI_CC_BREATHCNTRL_MSB 0x02
#define CSYS_MIDI_CC_FOOTCNTRL_MSB   0x04
#define CSYS_MIDI_CC_PORTAMENTO_MSB  0x05
#define CSYS_MIDI_CC_DATAENTRY_MSB   0x06
#define CSYS_MIDI_CC_CHANVOLUME_MSB  0x07
#define CSYS_MIDI_CC_BALANCE_MSB     0x08
#define CSYS_MIDI_CC_PAN_MSB         0x0A
#define CSYS_MIDI_CC_EXPRESSION_MSB  0x0B
#define CSYS_MIDI_CC_EFFECT1_MSB     0x0C
#define CSYS_MIDI_CC_EFFECT2_MSB     0x0D
#define CSYS_MIDI_CC_GEN1_MSB        0x10
#define CSYS_MIDI_CC_GEN2_MSB        0x11
#define CSYS_MIDI_CC_GEN3_MSB        0x12
#define CSYS_MIDI_CC_GEN4_MSB        0x13
#define CSYS_MIDI_CC_BANKSELECT_LSB  0x20
#define CSYS_MIDI_CC_MODWHEEL_LSB    0x21
#define CSYS_MIDI_CC_BREATHCNTRL_LSB 0x22
#define CSYS_MIDI_CC_FOOTCNTRL_LSB   0x24
#define CSYS_MIDI_CC_PORTAMENTO_LSB  0x25
#define CSYS_MIDI_CC_DATAENTRY_LSB   0x26
#define CSYS_MIDI_CC_CHANVOLUME_LSB  0x27
#define CSYS_MIDI_CC_BALANCE_LSB     0x28
#define CSYS_MIDI_CC_PAN_LSB         0x2A
#define CSYS_MIDI_CC_EXPRESSION_LSB  0x2B
#define CSYS_MIDI_CC_EFFECT1_LSB     0x2C
#define CSYS_MIDI_CC_EFFECT2_LSB     0x2D
#define CSYS_MIDI_CC_GEN1_LSB        0x30
#define CSYS_MIDI_CC_GEN2_LSB        0x31
#define CSYS_MIDI_CC_GEN3_LSB        0x32
#define CSYS_MIDI_CC_GEN4_LSB        0x33
#define CSYS_MIDI_CC_SUSTAIN         0x40
#define CSYS_MIDI_CC_PORTAMENTO      0x41
#define CSYS_MIDI_CC_SUSTENUTO       0x42
#define CSYS_MIDI_CC_SOFTPEDAL       0x43
#define CSYS_MIDI_CC_LEGATO          0x44
#define CSYS_MIDI_CC_HOLD2           0x45
#define CSYS_MIDI_CC_SOUNDCONTROL1   0x46
#define CSYS_MIDI_CC_SOUNDCONTROL2   0x47
#define CSYS_MIDI_CC_SOUNDCONTROL3   0x48
#define CSYS_MIDI_CC_SOUNDCONTROL4   0x49
#define CSYS_MIDI_CC_SOUNDCONTROL5   0x4A
#define CSYS_MIDI_CC_SOUNDCONTROL6   0x4B
#define CSYS_MIDI_CC_SOUNDCONTROL7   0x4C
#define CSYS_MIDI_CC_SOUNDCONTROL8   0x4D
#define CSYS_MIDI_CC_SOUNDCONTROL9   0x4E
#define CSYS_MIDI_CC_SOUNDCONTROL10  0x4F
#define CSYS_MIDI_CC_GEN5            0x50
#define CSYS_MIDI_CC_GEN6            0x51
#define CSYS_MIDI_CC_GEN7            0x52
#define CSYS_MIDI_CC_GEN8            0x53
#define CSYS_MIDI_CC_PORTAMENTOSRC   0x54
#define CSYS_MIDI_CC_EFFECT1DEPTH    0x5B
#define CSYS_MIDI_CC_EFFECT2DEPTH    0x5C
#define CSYS_MIDI_CC_EFFECT3DEPTH    0x5D
#define CSYS_MIDI_CC_EFFECT4DEPTH    0x5E
#define CSYS_MIDI_CC_EFFECT5DEPTH    0x5F
#define CSYS_MIDI_CC_DATAENTRYPLUS   0x60
#define CSYS_MIDI_CC_DATAENTRYMINUS  0x61
#define CSYS_MIDI_CC_ALLSOUNDOFF     0x78
#define CSYS_MIDI_CC_RESETALLCONTROL 0x79
#define CSYS_MIDI_CC_LOCALCONTROL    0x7A
#define CSYS_MIDI_CC_ALLNOTESOFF     0x7B


/*
#    Sfront, a SAOL to C translator    
#    This file: ascii kbd control driver for sfront
#    Copyright (C) 1999  Regents of the University of California
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License (Version 2) as
#    published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#    Maintainer: John Lazzaro, lazzaro@cs.berkeley.edu
*/

#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

/****************************************************************/
/****************************************************************/
/*             ascii keyboard control driver for sfront         */ 
/****************************************************************/

/****************************/
/* terminal and signal vars */
/****************************/

struct termios csysi_term_default;     /* to restore stdin */
sig_atomic_t csysi_no_interrupt = 1;   /* flags [cntrl-c]  */

/*************************/
/* keyboard input buffer */
/*************************/

#define CSYSI_INBUFF_SIZE 32

char csysi_inbuff[CSYSI_INBUFF_SIZE];   /* holds new keypresses */
int csysi_inbuff_cnt;                   
int csysi_inbuff_len;

/***********************/
/* keyboard action map */
/***********************/

#define CSYSI_MAPSIZE  256

typedef struct csysi_kinfo {
  unsigned char cmd;          /* CSYS_MIDI_{NOTEON,PROGRAM,CC,NOOP} */
  unsigned char ndata;        /* note number or parameter           */
} csysi_kinfo;

csysi_kinfo csysi_map[CSYSI_MAPSIZE];

/**********************/
/* current note state */
/**********************/

#define CSYSI_NOTESIZE 128
#define CSYSI_DURATION 0.1F

typedef struct csysi_noteinfo {

  unsigned char cmd;  
  float time;

} csysi_noteinfo;

csysi_noteinfo csysi_notestate[CSYSI_NOTESIZE];

int csysi_noteoff_min;   /* lowest pending noteoff     */
int csysi_noteoff_max;   /* highest pending noteoff    */
int csysi_noteoff_num;   /* number of pending noteoffs */

/****************/
/* timeout list */
/****************/

int csysi_noteready[CSYSI_NOTESIZE];
int csysi_noteready_len;

/***********/
/* volume  */
/***********/

#define CSYSI_VOLUME_DEFAULT 64
#define CSYSI_VOLUME_MAX 112
#define CSYSI_VOLUME_MIN 32
#define CSYSI_VOLUME_INCREMENT 8

int csysi_volume = CSYSI_VOLUME_DEFAULT;

/********************/
/* function externs */
/********************/

extern void csysi_signal_handler(int signum);
extern void csysi_kbdmap_init(void);


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*     high-level functions: called by sfront engine            */
/*______________________________________________________________*/

/****************************************************************/
/*             initialization routine for control               */
/****************************************************************/

int csys_setup(void)
     
{
  struct termios term_info;
  int i;

  /******************************/
  /* initialize data structures */
  /******************************/

  csysi_kbdmap_init();

  for (i = 0; i < CSYSI_NOTESIZE; i++)
    csysi_notestate[i].cmd = CSYS_MIDI_NOTEOFF;

  /*************************/
  /* set up signal handler */
  /*************************/

  if ((NSYS_NET == 0) && (signal(SIGINT, csysi_signal_handler) == SIG_ERR))
    {
      printf("Error: Can't set up SIGINT signal handler\n");
      return CSYS_ERROR;
    }

  /****************************************************/
  /* set up terminal: no echo, single-character reads */
  /****************************************************/

  if (tcgetattr(STDIN_FILENO, &csysi_term_default))
    {
      printf("Error: Can't set up terminal (1).\n");
      return CSYS_ERROR;
    }

  term_info = csysi_term_default;

  term_info.c_lflag &= (~ICANON);   
  term_info.c_lflag &= (~ECHO);
  term_info.c_cc[VTIME] = 0;        
  term_info.c_cc[VMIN] = 0;

  if (tcsetattr(STDIN_FILENO, TCSANOW, &term_info))
    {
      printf("Error: Can't set up terminal (2).\n");
      return CSYS_ERROR;
    }

  if (fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK))
    {
      printf("Error: Can't set stdin to O_NONBLOCK.\n");
      return CSYS_ERROR;
    }

  /*********************/
  /* user instructions */
  /*********************/

#if (!defined(ASYS_OUTDRIVER_LINUX))

  fprintf(stderr, 
	  "\nInput Driver Instructions for -cin ascii:\n");
  fprintf(stderr, 
	  "{a-z}: notes, {0-9}: MIDI presets, {+,-} volume, cntrl-C exits.\n");

#if (!defined(ASYS_OUTDRIVER_COREAUDIO))
  fprintf(stderr, 
	  "If autorepeat interferes, try 'xset -r' to disable it.\n\n");
#endif

#endif
  
  return CSYS_DONE;
}

/****************************************************************/
/*             polling routine for new data                     */
/****************************************************************/

int csys_newdata(void)
     
{
  int i, bottom, top;
  int ret = CSYS_NONE;

  if (csysi_no_interrupt)
    {
      /* see if any NoteOn's ready to be turned off */

      if (csysi_noteoff_num)
	{
	  csysi_noteready_len = 0;
	  top = bottom = -1;
	  for (i = csysi_noteoff_min; i <= csysi_noteoff_max; i++)
	    if (csysi_notestate[i].cmd == CSYS_MIDI_NOTEON)
	      {
		if (csysi_notestate[i].time > absolutetime)
		  {
		    if (bottom < 0)
		      bottom = i;
		    top = i;
		  }
		else
		  {
		    csysi_notestate[i].cmd = CSYS_MIDI_NOTEOFF;
		    csysi_noteready[csysi_noteready_len++] = i;
		    csysi_noteoff_num--;
		    ret = CSYS_MIDIEVENTS;
		  }
	      }
	  csysi_noteoff_min = bottom;
	  csysi_noteoff_max = top;
	}

      /* check for new keypresses */

      csysi_inbuff_cnt = 0;
      csysi_inbuff_len = read(STDIN_FILENO, csysi_inbuff, CSYSI_INBUFF_SIZE);
      csysi_inbuff_len = (csysi_inbuff_len >= 0) ? csysi_inbuff_len : 0;
      return (csysi_inbuff_len ? CSYS_MIDIEVENTS : ret);
    }
  else
    return CSYS_MIDIEVENTS;
}

/****************************************************************/
/*                 processes a MIDI event                       */
/****************************************************************/

int csys_midievent(unsigned char * cmd,   unsigned char * ndata, 
	           unsigned char * vdata, unsigned short * extchan,
		   float * fval)

{
  unsigned char i;

  if (csysi_no_interrupt)
    {
      *extchan = 0;

      /* check for new NoteOffs */

      if (csysi_noteready_len)
	{
	  csysi_noteready_len--;
	  *cmd = CSYS_MIDI_NOTEOFF;
	  *ndata = csysi_noteready[csysi_noteready_len];
	  *vdata = 0;
	  return ((csysi_inbuff_len || csysi_noteready_len) ? 
		  CSYS_MIDIEVENTS : CSYS_NONE);
	}

      /* handle new keypresses */

      *cmd = CSYS_MIDI_NOOP;

      while (csysi_inbuff_cnt < csysi_inbuff_len)
	switch (csysi_map[(i = csysi_inbuff[csysi_inbuff_cnt++])].cmd) {
	case CSYS_MIDI_NOTEON:

	  *ndata = csysi_map[i].ndata;

	  if (csysi_notestate[*ndata].cmd == CSYS_MIDI_NOTEOFF)
	    {
	      *cmd = CSYS_MIDI_NOTEON;
	      *vdata = csysi_volume;
	      
	      csysi_notestate[*ndata].cmd = CSYS_MIDI_NOTEON;
	      csysi_notestate[*ndata].time = absolutetime + CSYSI_DURATION;
	      
	      if (csysi_noteoff_num++)
		{
		  if (*ndata > csysi_noteoff_max)
		    csysi_noteoff_max = *ndata;
		  else
		    if (*ndata < csysi_noteoff_min)
		      csysi_noteoff_min = *ndata;
		}
	      else
		csysi_noteoff_min = csysi_noteoff_max = *ndata;
	      return ((csysi_inbuff_cnt < csysi_inbuff_len) ? 
		      CSYS_MIDIEVENTS : CSYS_NONE);
	    }

	  break;
	case CSYS_MIDI_PROGRAM:
	  *cmd = CSYS_MIDI_PROGRAM;
	  *ndata = csysi_map[i].ndata;
	  return ((csysi_inbuff_cnt < csysi_inbuff_len) ? 
		  CSYS_MIDIEVENTS : CSYS_NONE);
	  break;	
	case CSYS_MIDI_CC:
	  if (csysi_map[i].ndata)
	    {
	      csysi_volume += CSYSI_VOLUME_INCREMENT;
	      if (csysi_volume > CSYSI_VOLUME_MAX)
		csysi_volume = CSYSI_VOLUME_MAX;
	    }
	  else
	    {
	      csysi_volume -= CSYSI_VOLUME_INCREMENT;
	      if (csysi_volume < CSYSI_VOLUME_MIN)
		csysi_volume = CSYSI_VOLUME_MIN;
	    }
	  break;	
	case CSYS_MIDI_NOOP:
	  break;
	default:
	  break;
	}

      return CSYS_NONE;
    }

  /* end session if a cntrl-C clears csysi_no_interrupt */

  *cmd = CSYS_MIDI_ENDTIME;
  *fval = scorebeats;
  return CSYS_NONE;
}

/****************************************************************/
/*                  closing routine for control                 */
/****************************************************************/

void csys_shutdown(void)
     
{
  tcsetattr(STDIN_FILENO, TCSANOW, &csysi_term_default);
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*   mid-level functions: called by top-level driver functions  */
/*______________________________________________________________*/

/****************************************************************/
/*             initializes keyboard map                         */
/****************************************************************/

void csysi_kbdmap_init(void)

{
  int i;

  /* MIDI note number map */

  csysi_map['z'].ndata = csysi_map['a'].ndata = csysi_map['q'].ndata = 48; 
  csysi_map['Z'].ndata = csysi_map['A'].ndata = csysi_map['Q'].ndata = 48; 
  csysi_map['x'].ndata = csysi_map['s'].ndata = csysi_map['w'].ndata = 50; 
  csysi_map['X'].ndata = csysi_map['S'].ndata = csysi_map['W'].ndata = 50; 
  csysi_map['c'].ndata = csysi_map['d'].ndata = csysi_map['e'].ndata = 52; 
  csysi_map['C'].ndata = csysi_map['D'].ndata = csysi_map['E'].ndata = 52; 
  csysi_map['v'].ndata = csysi_map['f'].ndata = csysi_map['r'].ndata = 55; 
  csysi_map['V'].ndata = csysi_map['F'].ndata = csysi_map['R'].ndata = 55; 
  csysi_map['b'].ndata = csysi_map['g'].ndata = csysi_map['t'].ndata = 57;
  csysi_map['B'].ndata = csysi_map['G'].ndata = csysi_map['T'].ndata = 57;
  csysi_map['n'].ndata = csysi_map['h'].ndata = csysi_map['y'].ndata = 60; 
  csysi_map['N'].ndata = csysi_map['H'].ndata = csysi_map['Y'].ndata = 60; 
  csysi_map['m'].ndata = csysi_map['j'].ndata = csysi_map['u'].ndata = 62; 
  csysi_map['M'].ndata = csysi_map['J'].ndata = csysi_map['U'].ndata = 62; 
  csysi_map[','].ndata = csysi_map['k'].ndata = csysi_map['i'].ndata = 64; 
  csysi_map['<'].ndata = csysi_map['K'].ndata = csysi_map['I'].ndata = 64; 
  csysi_map['.'].ndata = csysi_map['l'].ndata = csysi_map['o'].ndata = 67; 
  csysi_map['>'].ndata = csysi_map['L'].ndata = csysi_map['O'].ndata = 67; 
  csysi_map['/'].ndata = csysi_map[';'].ndata = csysi_map['p'].ndata = 69;
  csysi_map['\?'].ndata = csysi_map[':'].ndata = csysi_map['P'].ndata = 69;
  csysi_map['\''].ndata = csysi_map['['].ndata = 72; 
  csysi_map['"'].ndata = csysi_map['{'].ndata = 72; 
  csysi_map[']'].ndata = 74; 
  csysi_map['}'].ndata = 74; 
  csysi_map['\\'].ndata = csysi_map['\n'].ndata = 76; 
  csysi_map['|'].ndata = 76; 
  csysi_map[' '].ndata = 48;

  /* set command type */

  for (i = 0; i < CSYSI_MAPSIZE; i++)
    {
      if (csysi_map[i].ndata)
	csysi_map[i].cmd = CSYS_MIDI_NOTEON;
      else
	if (isdigit(i))
	  {
	    csysi_map[i].cmd = CSYS_MIDI_PROGRAM;
	    csysi_map[i].ndata = (i - '0');
	  }
	else
	  if ((i == '+') || (i == '_') || (i == '-') || (i == '='))
	    {
	      csysi_map[i].cmd = CSYS_MIDI_CC;
	      csysi_map[i].ndata = ((i == '+') || (i == '='));
	    }
	  else
	    csysi_map[i].cmd = CSYS_MIDI_NOOP;
    }

}



/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                 low-level functions                          */
/*______________________________________________________________*/

/****************************************************************/
/*                 SIGINT signal handler                        */
/****************************************************************/

void csysi_signal_handler(int signum)

{

  if (csysi_no_interrupt)
    {
      csysi_no_interrupt = 0;
    }
  else
    {
      exit(129); /* emergency shutdown */
    }

}




#undef NS
#define NS(x) nstate->x
#undef NSP
#define NSP nstate
#undef NT
#define NT(x)  nstate->t[x]
#undef NV
#define NV(x)  nstate->v[x].f
#undef NVI
#define NVI(x)  nstate->v[x].i
#undef NVU
#define NVU(x)  nstate->v[x]
#undef NP
#define NP(x)  nstate->v[x].f
#undef NPI
#define NPI(x)  nstate->v[x].i

void sine_ipass(struct ninstr_types * nstate)
{
   int i;

memset(&(NV(0)), 0, sine_ENDVAR*sizeof(float));
memset(&(NT(0)), 0, sine_ENDTBL*sizeof(struct tableinfo));
   NV(sine_pitch) = 
   NS(iline->p[sine_pitch]);
   NV(sine_vel) = 
   NS(iline->p[sine_vel]);
NV(sine_a) =  2.0F  * ((float)sin( 7.123788e-05F  * (globaltune*(float)(pow(2.0F, 8.333334e-02F*(-69.0F + NV(sine_pitch))))))
);
 NV(sine_vel) =  3.100006e-05F  * NV(sine_vel);
 NV(sine__tvr0) = NV(sine_vel) *  2.267574e-05F ;
 NV(sine__tvr1) = NV(sine_vel) *  9.523810e-04F ;
 
}


#undef NS
#define NS(x) nstate->x
#undef NSP
#define NSP nstate
#undef NT
#define NT(x)  nstate->t[x]
#undef NV
#define NV(x)  nstate->v[x].f
#undef NVI
#define NVI(x)  nstate->v[x].i
#undef NVU
#define NVU(x)  nstate->v[x]
#undef NP
#define NP(x)  nstate->v[x].f
#undef NPI
#define NPI(x)  nstate->v[x].i

void sine_kpass(struct ninstr_types * nstate)
{

   int i;

   NS(iline->itime) = ((float)(kcycleidx - NS(iline->kbirth)))*KTIME;

NV(sine_incr) =  0.0F ;
  if  (  ((float)NS(iline->released))  &&  ! NV(sine_rel))
 { NV(sine_rel) =  1.0F ;
   if ((NS(iline->sdur) < 0.0F)||
   (NS(iline->turnoff)&&NS(iline->released)))
  {
    NS(iline->endtime) = scorebase;
    NS(iline->endabs) = (kbase - 1)*KTIME;
    NS(iline->sdur) = 0.0F;
  }
  NS(iline->abstime) += 
 6.000000e-02F   ;
   if (NS(iline->released))
    NS(iline->turnoff) = 0;
}
 if  (  (  ! NV(sine_rel))
 &&  (  NS(iline->itime)  <  4.285715e-03F )
)
 { NV(sine_incr) =  NG(259*NS(iline->numchan) + 0 +  7  )  * NV(sine__tvr0) *  2.100000e+02F ;
 NV(sine_tot) = NV(sine_tot) +  NG(259*NS(iline->numchan) + 0 +  7  )  * NV(sine__tvr1) *  2.100000e+02F ;
 }
 if  ( NV(sine_rel))
 { NV(sine_incr) =  - NV(sine_tot) *  2.267574e-05F  *  1.615385e+01F ;
 }

}


#undef NS
#define NS(x) nstate->x
#undef NSP
#define NSP nstate
#undef NT
#define NT(x)  nstate->t[x]
#undef NV
#define NV(x)  nstate->v[x].f
#undef NVI
#define NVI(x)  nstate->v[x].i
#undef NVU
#define NVU(x)  nstate->v[x]
#undef NP
#define NP(x)  nstate->v[x].f
#undef NPI
#define NPI(x)  nstate->v[x].i

void sine_apass(struct ninstr_types * nstate)
{

 if  ( NV(sine_init) ==  0.0F )
 { NV(sine_x) =  0.25F ;
 NV(sine_init) =  1.0F ;
 }
NV(sine_x) = NV(sine_x) - NV(sine_a) * NV(sine_y);
 NV(sine_y) = NV(sine_y) + NV(sine_a) * NV(sine_x);
 NV(sine_env) = NV(sine_env) + NV(sine_incr);
 TB(BUS_output_bus + 0) += NV(sine_env) * NV(sine_y);
}


#undef NS
#define NS(x) nstate->x
#undef NSP
#define NSP nstate
#undef NT
#define NT(x)  nstate->t[x]
#undef NV
#define NV(x)  nstate->v[x].f
#undef NVI
#define NVI(x)  nstate->v[x].i
#undef NVU
#define NVU(x)  nstate->v[x]
#undef NP
#define NP(x)  nstate->v[x].f
#undef NPI
#define NPI(x)  nstate->v[x].i


#undef NS
#define NS(x) 0
#undef NSP
#define NSP NULL
#undef NT
#define NT(x)  gtables[x]
#undef NV
#define NV(x)  global[x].f
#undef NVI
#define NVI(x)  global[x].i
#undef NVU
#define NVU(x)  global[x]
#undef NP
#define NP(x)  global[x].f
#undef NPI
#define NPI(x)  global[x].i


#undef NS
#define NS(x) nstate->x
#undef NSP
#define NSP nstate
#undef NT
#define NT(x)  nstate->t[x]
#undef NV
#define NV(x)  nstate->v[x].f
#undef NVI
#define NVI(x)  nstate->v[x].i
#undef NVU
#define NVU(x)  nstate->v[x]
#undef NP
#define NP(x)  nstate->v[x].f
#undef NPI
#define NPI(x)  nstate->v[x].i



#undef NS
#define NS(x) 0
#undef NSP
#define NSP NULL
#undef NT
#define NT(x)  gtables[x]
#undef NV
#define NV(x)  global[x].f
#undef NVI
#define NVI(x)  global[x].i
#undef NVU
#define NVU(x)  global[x]
#undef NP
#define NP(x)  global[x].f
#undef NPI
#define NPI(x)  global[x].i



void system_init(int argc, char **argv)
{

   int i;


   srand(((unsigned int)time(0))|1);
   asys_argc = argc;
   asys_argv = argv;

   csys_argc = argc;
   csys_argv = argv;


    /* MIDI volume */

   global[7].f = global[266].f = global[525].f = global[784].f = 
   global[1043].f = global[1302].f = global[1561].f = global[1820].f = 
   global[2079].f = global[2338].f = global[2597].f = global[2856].f = 
   global[3115].f = global[3374].f = global[3633].f = global[3892].f = 
    100.0F; 

    /* MIDI Pan */

   global[10].f = global[269].f = global[528].f = global[787].f = 
   global[1046].f = global[1305].f = global[1564].f = global[1823].f = 
   global[2082].f = global[2341].f = global[2600].f = global[2859].f = 
   global[3118].f = global[3377].f = global[3636].f = global[3895].f = 
    64.0F; 

    /* MIDI Expression */

   global[11].f = global[270].f = global[529].f = global[788].f = 
   global[1047].f = global[1306].f = global[1565].f = global[1824].f = 
   global[2083].f = global[2342].f = global[2601].f = global[2860].f = 
   global[3119].f = global[3378].f = global[3637].f = global[3896].f = 
    127.0F; 

    /* MIDI Bend */

   global[257].f = global[516].f = global[775].f = global[1034].f = 
   global[1293].f = global[1552].f = global[1811].f = global[2070].f = 
   global[2329].f = global[2588].f = global[2847].f = global[3106].f = 
   global[3365].f = global[3624].f = global[3883].f = global[4142].f = 
    8192.0F; 

    /* MIDI Ext Channel Number */

   global[258].f = 0.0F; global[517].f = 1.0F; global[776].f = 2.0F; 
   global[1035].f = 3.0F; global[1294].f = 4.0F; global[1553].f = 5.0F; 
   global[1812].f = 6.0F; global[2071].f = 7.0F; global[2330].f = 8.0F; 
   global[2589].f = 9.0F; global[2848].f = 10.0F; global[3107].f = 11.0F; 
   global[3366].f = 12.0F; global[3625].f = 13.0F; global[3884].f = 14.0F; 
   global[4143].f = 15.0F; 

   memset(&(NV(GBL_STARTVAR)), 0, 
          (GBL_ENDVAR-GBL_STARTVAR)*sizeof(float));
   memset(&(NT(0)), 0, GBL_ENDTBL*sizeof(struct tableinfo));

#undef NS
#define NS(x) nstate->x
#undef NSP
#define NSP nstate
#undef NT
#define NT(x)  nstate->t[x]
#undef NV
#define NV(x)  nstate->v[x].f
#undef NVI
#define NVI(x)  nstate->v[x].i
#undef NVU
#define NVU(x)  nstate->v[x]
#undef NP
#define NP(x)  nstate->v[x].f
#undef NPI
#define NPI(x)  nstate->v[x].i

   for (busidx=0; busidx<ENDBUS;busidx++)
      TB(busidx)=0.0F;


}


#undef NS
#define NS(x) 0
#undef NSP
#define NSP NULL
#undef NT
#define NT(x)  gtables[x]
#undef NV
#define NV(x)  global[x].f
#undef NVI
#define NVI(x)  global[x].i
#undef NVU
#define NVU(x)  global[x]
#undef NP
#define NP(x)  global[x].f
#undef NPI
#define NPI(x)  global[x].i



void effects_init(void)
{



}


#undef NS
#define NS(x) nstate->x
#undef NSP
#define NSP nstate
#undef NT
#define NT(x)  nstate->t[x]
#undef NV
#define NV(x)  nstate->v[x].f
#undef NVI
#define NVI(x)  nstate->v[x].i
#undef NVU
#define NVU(x)  nstate->v[x]
#undef NP
#define NP(x)  nstate->v[x].f
#undef NPI
#define NPI(x)  nstate->v[x].i


#undef NS
#define NS(x) 0
#undef NSP
#define NSP NULL
#undef NT
#define NT(x)  gtables[x]
#undef NV
#define NV(x)  global[x].f
#undef NVI
#define NVI(x)  global[x].i
#undef NVU
#define NVU(x)  global[x]
#undef NP
#define NP(x)  global[x].f
#undef NPI
#define NPI(x)  global[x].i



void shut_down(void)
   {

  if (graceful_exit)
    {
      fprintf(stderr, "\nShutting down system ... please wait.\n");
      fprintf(stderr, "If no termination in 10 seconds, use Ctrl-C or Ctrl-\\ to force exit.\n");
      fflush(stderr);
    }
   asys_putbuf(&asys_obuf, &obusidx);
   asys_oshutdown();

   csys_shutdown();

   }

void main_apass(void)

{

 for (sysidx=cm_sine___first;sysidx<=cm_sine___last;sysidx++)
  if (sysidx->noteon == PLAYING)
   sine_apass(sysidx->nstate);

}

int main_kpass(void)

{

 for (sysidx=cm_sine___first;sysidx<=cm_sine___last;sysidx++)
  if (sysidx->noteon == PLAYING)
  {
   sine_kpass(sysidx->nstate);
  }

  return graceful_exit;
}

void main_ipass(void)

{

int i;

   unsigned char  ndata, vdata;
   unsigned short ec;
   float oldtime;
   struct instr_line * oldest;
   int ne, newevents, netevents;
   unsigned char cmd;
   float fval;

   newevents = csys_newdata();
   if (newevents & CSYS_MIDIEVENTS)
   do
    {
     ne = csys_midievent(&cmd,&ndata,&vdata,&ec,&fval);
     if ((ec < CSYS_MAXEXTCHAN)||((cmd&0xF0)==CSYS_MIDI_SPECIAL))
     switch(cmd&0xF0) {
      case CSYS_MIDI_NOTEON:
      if ((cme_first[ec])==NULL)
        break;
      if (vdata != 0)
       {
        if (*cme_next[ec] == NULL)
          *cme_next[ec] = *cme_first[ec] = *cme_last[ec];
        else
          {
            oldest = NULL;
            oldtime = MAXENDTIME;
            *cme_next[ec] = *cme_first[ec];
            while ((*cme_next[ec]) < (*cme_end[ec]))
      	 {
      	   if (((*cme_next[ec])->noteon == NOTUSEDYET)||
      	       ((*cme_next[ec])->noteon == ALLDONE))
      	     break;
               if ((*cme_next[ec])->starttime < oldtime)
                {
                  oldest =  (*cme_next[ec]);
                  oldtime = (*cme_next[ec])->starttime;
                }
      	   (*cme_next[ec])++;
      	 }
           if ((*cme_next[ec]) == (*cme_end[ec]))
            {
              *cme_next[ec] = oldest;
              oldest->nstate->iline = NULL;
            }
            if ((*cme_next[ec]) > (*cme_last[ec]))
      	    *cme_last[ec] = *cme_next[ec];
          }
        (*cme_next[ec])->starttime = scorebeats - scoremult;
        (*cme_next[ec])->endtime = MAXENDTIME;
        (*cme_next[ec])->abstime = 0.0F;
        (*cme_next[ec])->time = 0.0F;
        (*cme_next[ec])->sdur = -1.0F;
        (*cme_next[ec])->released = 0;
        (*cme_next[ec])->turnoff = 0;
        (*cme_next[ec])->noteon = TOBEPLAYED;
        (*cme_next[ec])->numchan = ec + CSYS_EXTCHANSTART;
        (*cme_next[ec])->preset  = cme_preset[ec];
        (*cme_next[ec])->notenum  = ndata;
        (*cme_next[ec])->p[0] = ndata ;
        (*cme_next[ec])->p[1] = vdata ;
        break;
       }
      case CSYS_MIDI_NOTEOFF:
       if ((cme_first[ec])==NULL)
         break;
       oldest = NULL;
       oldtime = MAXENDTIME;
       for (sysidx=*cme_first[ec];sysidx<=*cme_last[ec];sysidx++)
         if ((sysidx->notenum == ndata) && 
             (sysidx->numchan == ec + CSYS_EXTCHANSTART) &&
             (sysidx->starttime < oldtime))
          {
            oldest = sysidx;
            oldtime = sysidx->starttime;
          }
       if (oldest)
          {
             oldest->endtime = scorebeats - scoremult;
             oldest->notenum += 256;
          }
       break;
      case CSYS_MIDI_PTOUCH:
       NG(CSYS_FRAMELEN*(ec+CSYS_EXTCHANSTART)+CSYS_TOUCHPOS+ndata)=vdata;
      break;
      case CSYS_MIDI_CTOUCH:
       NG(CSYS_FRAMELEN*(ec+CSYS_EXTCHANSTART)+CSYS_CHTOUCHPOS)=vdata;
      break;
      case CSYS_MIDI_WHEEL:
       NG(CSYS_FRAMELEN*(ec + CSYS_EXTCHANSTART) + CSYS_BENDPOS) = 
                                               128*vdata + ndata;
      break;
      case CSYS_MIDI_CC:
       NG(CSYS_FRAMELEN*(ec+CSYS_EXTCHANSTART)+CSYS_CCPOS+ndata)=vdata;
       switch(ndata) {
        case CSYS_MIDI_CC_BANKSELECT_LSB:
         csys_banklsb = vdata;
         csys_bank = csys_banklsb + 128*csys_bankmsb;
         break;
        case CSYS_MIDI_CC_BANKSELECT_MSB:
         csys_bankmsb = vdata;
         csys_bank = csys_banklsb + 128*csys_bankmsb;
         break;
        case CSYS_MIDI_CC_ALLSOUNDOFF:
          if ((cme_first[ec]))
           for (sysidx=*cme_first[ec];sysidx<=*cme_last[ec];sysidx++)
            {
               sysidx->noteon = ALLDONE;
               sysidx->nstate->iline = NULL;
            }
         break;
        case CSYS_MIDI_CC_ALLNOTESOFF:
          if ((cme_first[ec]))
           for (sysidx=*cme_first[ec];sysidx<=*cme_last[ec];sysidx++)
            {
               sysidx->endtime =  scorebeats - scoremult;
               sysidx->notenum += 256;
            }
         break;
        default:
         break;
        }
      break;
      case CSYS_MIDI_PROGRAM:
       if ((CSYS_NULLPROGRAM == 0) && (ndata >= CSYS_MAXPRESETS))
         break;
       if ((cme_first[ec]))
        for (sysidx=*cme_first[ec];sysidx<=*cme_last[ec];sysidx++)
          if ((sysidx->numchan == ec + CSYS_EXTCHANSTART) &&
             (sysidx->noteon <= PLAYING))
           {
              sysidx->endtime = scorebeats - scoremult;
              sysidx->notenum += 256;
           }
       if ((CSYS_NULLPROGRAM == 0) || (ndata < CSYS_MAXPRESETS))
         {
          cme_first[ec] = cmp_first[ndata];
          cme_last[ec] = cmp_last[ndata];
          cme_end[ec] = cmp_end[ndata];
          cme_next[ec] = cmp_next[ndata];
         }
       else
         cme_first[ec] = cme_last[ec] = cme_end[ec] = cme_next[ec] = NULL;
      break;
      case CSYS_MIDI_SPECIAL:
       if (cmd == CSYS_MIDI_NEWTEMPO)
        {
         if (fval <= 0.0F)
           break;
         kbase = kcycleidx;
         scorebase = scorebeats;
         tempo = fval;
         scoremult = 1.666667e-02F*KTIME*tempo;
         endkcycle = kbase + (int)(KRATE*(endtime-scorebase)*
                     (60.0F/tempo));
         break;
        }
       if (cmd == CSYS_MIDI_ENDTIME)
        {
         if (fval <= scorebeats)
         {
           endtime = scorebeats;
           endkcycle = kcycleidx;
         }
         else
         {
           endtime = fval;
           endkcycle = kbase + (int)(KRATE*(endtime-scorebase)*
                       (60.0F/tempo));
         }
         break;
        }
       if (cmd == CSYS_MIDI_POWERUP)
        {
         ec &= 0xFFF0;
         memset(&(NG(CSYS_FRAMELEN*(ec+CSYS_EXTCHANSTART))), 0,
                sizeof(float)*CSYS_FRAMELEN*16);
         for (i = 0; i < 16; i++)
         {
           NG(CSYS_FRAMELEN*(ec + CSYS_EXTCHANSTART)+
              CSYS_MIDI_CC_CHANVOLUME_MSB) = 100.0F;
           NG(CSYS_FRAMELEN*(ec + CSYS_EXTCHANSTART)+
              CSYS_MIDI_CC_PAN_MSB) = 64.0F;
           NG(CSYS_FRAMELEN*(ec + CSYS_EXTCHANSTART)+
              CSYS_MIDI_CC_EXPRESSION_MSB) = 127.0F;
           NG(CSYS_FRAMELEN*(ec + CSYS_EXTCHANSTART)+
              CSYS_BENDPOS) = 8192.0F;
           NG(CSYS_FRAMELEN*(ec + CSYS_EXTCHANSTART)+
              CSYS_EXTPOS) = (float)ec;
           if ((cme_first[ec]))
            for (sysidx=*cme_first[ec];sysidx<=*cme_last[ec];sysidx++)
             if (sysidx->numchan == (ec + CSYS_EXTCHANSTART))
               {
                  sysidx->endtime =  scorebeats - scoremult;
                  sysidx->notenum += 256;
               }
           cme_first[ec] = cmp_first[i % CSYS_MAXPRESETS];
           cme_last[ec] = cmp_last[i % CSYS_MAXPRESETS];
           cme_end[ec] = cmp_end[i % CSYS_MAXPRESETS];
           cme_next[ec] = cmp_next[i % CSYS_MAXPRESETS];
           ec++;
         }
         break;
        }
      break;
      default:
      break;
     }
    } while (ne != CSYS_DONE);

 for (sysidx=cm_sine___first;sysidx<=cm_sine___last;sysidx++)
  {
  switch(sysidx->noteon) {
   case PLAYING:
   if (sysidx->released)
    {
     if (sysidx->turnoff)
      {
        sysidx->noteon = ALLDONE;
        for (i = 0; i < sine_ENDTBL; i++)
         if (sysidx->nstate->t[i].llmem)
           free(sysidx->nstate->t[i].t);
        sysidx->nstate->iline = NULL;
      }
     else
      {
        sysidx->abstime -= KTIME;
        if (sysidx->abstime < 0.0F)
         {
           sysidx->noteon = ALLDONE;
           for (i = 0; i < sine_ENDTBL; i++)
            if (sysidx->nstate->t[i].llmem)
             free(sysidx->nstate->t[i].t);
           sysidx->nstate->iline = NULL;
         }
        else
         sysidx->turnoff = sysidx->released = 0;
      }
    }
   else
    {
     if (sysidx->turnoff)
      {
       sysidx->released = 1;
      }
     else
      {
        if ((sysidx->endtime <= scorebeats) &&
        (NG(259*sysidx->nstate->iline->numchan+64) == 0.0F))
         {
           if (sysidx->abstime <= 0.0F)
             sysidx->turnoff = sysidx->released = 1;
           else
           {
             sysidx->abstime -= KTIME;
             if (sysidx->abstime < 0.0F)
               sysidx->turnoff = sysidx->released = 1;
           }
         }
        else
          if ((sysidx->abstime < 0.0F) &&
           (1.666667e-2F*tempo*sysidx->abstime + 
               sysidx->endtime <= scorebeats))
            sysidx->turnoff = sysidx->released = 1;
      }
    }
   break;
   case TOBEPLAYED:
    if (sysidx->starttime <= scorebeats)
     {
      sysidx->noteon = PLAYING;
      sysidx->notestate = nextstate;
      sysidx->nstate = &ninstr[nextstate];
      if (sysidx->sdur >= 0.0F)
        sysidx->endtime = scorebeats + sysidx->sdur;
      sysidx->kbirth = kcycleidx;
      ninstr[nextstate].iline = sysidx;
      sysidx->time = (kcycleidx-1)*KTIME;
       oldstate = nextstate;
       nextstate = (nextstate+1) % MAXSTATE;
       while ((oldstate != nextstate) && 
              (ninstr[nextstate].iline != NULL))
         nextstate = (nextstate+1) % MAXSTATE;
       if (oldstate == nextstate)
       {
         nextstate = (nextstate+1) % MAXSTATE;
         while ((oldstate != nextstate) && 
             (ninstr[nextstate].iline->time == 0.0F) &&
             (ninstr[nextstate].iline->noteon == PLAYING))
           nextstate = (nextstate+1) % MAXSTATE;
         ninstr[nextstate].iline->noteon = ALLDONE;
         ninstr[nextstate].iline = NULL;
       }
      sine_ipass(sysidx->nstate);
    }
   break;
   default:
   break;
   }
 }
  while (cm_sine___last->noteon == ALLDONE)
   if (cm_sine___last == cm_sine___first)
    {
     cm_sine___first = &cm_sine[1];
     cm_sine___last =  &cm_sine[0];
     cm_sine___next =  NULL;
     cm_sine___last->noteon = TOBEPLAYED;
     break;
    }
   else
    cm_sine___last--;

}

void main_initpass(void)

{


   if (csys_setup() != CSYS_DONE)
   epr(0,NULL,NULL,"control input device unavailable");

   if (asys_osetup((int)ARATE, ASYS_OCHAN, ASYS_OTYPENAME,  "linux",
ASYS_TIMEOPTION) != ASYS_DONE)
   epr(0,NULL,NULL,"audio output device unavailable");
   endkcycle = kbase + (int) 
      (KRATE*(endtime - scorebase)*(60.0F/tempo));

   if (asys_preamble(&asys_obuf, &asys_osize) != ASYS_DONE)
   epr(0,NULL,NULL,"audio output device unavailable");
   ksyncinit();


}

void main_control(void)

{

}


/*
#    Sfront, a SAOL to C translator    
#    This file: Robust file I/O, termination function
#    Copyright (C) 1999  Regents of the University of California
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License (Version 2) as
#    published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#    Primary Author: John Lazzaro, lazzaro@cs.berkeley.edu
*/

/* handles termination in case of error */

void epr(int linenum, char * filename, char * token, char * message)

{

  fprintf(stderr, "\nRuntime Error.\n");
  if (filename != NULL)
    fprintf(stderr, "Location: File %s near line %i.\n",filename, linenum);
  if (token != NULL)
    fprintf(stderr, "While executing: %s.\n",token);
  if (message != NULL)
    fprintf(stderr, "Potential problem: %s.\n",message);
  fprintf(stderr, "Exiting program.\n\n");
  exit(-1);

}

/* robust replacement for fread() */

size_t rread(void * ptr, size_t size, size_t nmemb, FILE * stream)

{
  long recv;
  long len;
  long retry;
  char * c;

  /* fast path */

  if ((recv = fread(ptr, size, nmemb, stream)) == nmemb)
    return nmemb;

  /* error recovery */
     
  c = (char *) ptr;
  len = retry = 0;

  do 
    {
      if (++retry > IOERROR_RETRY)
	{
	  len = recv = 0;
	  break;
	}

      if (feof(stream))
	{
	  clearerr(stream);
	  break;
	}

      /* ANSI, not POSIX, so can't look for EINTR/EAGAIN  */
      /* Assume it was one of these and retry.            */

      clearerr(stream);
      len += recv;
      nmemb -= recv;

    }
  while ((recv = fread(&(c[len]), size, nmemb, stream)) != nmemb);

  return (len += recv);

}

/* robust replacement for fwrite() */

size_t rwrite(void * ptr, size_t size, size_t nmemb, FILE * stream)

{
  long recv;
  long len;
  long retry;
  char * c;

  /* fast path */

  if ((recv = fwrite(ptr, size, nmemb, stream)) == nmemb)
    return nmemb;

  /* error recovery */
     
  c = (char *) ptr;
  len = retry = 0;

  do 
    {
      if (++retry > IOERROR_RETRY)
	{
	  len = recv = 0;
	  break;
	}

      /* ANSI, not POSIX, so can't look for EINTR/EAGAIN  */
      /* Assume it was one of these and retry.            */

      len += recv;
      nmemb -= recv;

    }
  while ((recv = fwrite(&(c[len]), size, nmemb, stream)) != nmemb);

  return (len += recv);

}



/*
#    Sfront, a SAOL to C translator    
#    This file: Main loop for runtime
#    Copyright (C) 1999  Regents of the University of California
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License (Version 2) as
#    published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#    Primary Author: John Lazzaro, lazzaro@cs.berkeley.edu
*/

int main(int argc, char *argv[])

{
  system_init(argc, argv);
  effects_init();
  main_initpass();
  for (kcycleidx=kbase; kcycleidx<=endkcycle; kcycleidx++)
    {
      pass = IPASS;
      scorebeats = scoremult*(kcycleidx - kbase) + scorebase;
      absolutetime = (kcycleidx - 1)*KTIME;
      main_ipass();
      pass = KPASS;
      main_control();
      if (main_kpass())
	break;
      pass = APASS;
      for (acycleidx=0; acycleidx<ACYCLE; acycleidx++)
	{
	  for (busidx=0; busidx<ENDBUS;busidx++)
	    bus[busidx]=0.0F;
	  main_apass();
	  for (busidx=BUS_output_bus; busidx<ENDBUS_output_bus;busidx++)
	    {
	      bus[busidx] = (bus[busidx] >  1.0F) ?  1.0F : bus[busidx];
	      bus[busidx] = (bus[busidx] < -1.0F) ? -1.0F : bus[busidx];
	      asys_obuf[obusidx++] = (short) (32767.0F * bus[busidx]);
	    }
	  if (obusidx >= asys_osize)
	    {
	      obusidx = 0;
	      if (asys_putbuf(&asys_obuf, &asys_osize))
		{
		  fprintf(stderr,"  Sfront: Audio output device problem\n\n");
		  kcycleidx = endkcycle;
		  break;
		}
	    }
	}
      acycleidx = 0; 
      cpuload = ksync();
    }
  shut_down();
  return 0;
}





