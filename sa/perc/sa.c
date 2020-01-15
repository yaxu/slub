


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


int csys_sfront_argc = 11;

char * csys_sfront_argv[11] = {
"sfront",
"-latency",
"0.002",
"-aout",
"linux",
"-cin",
"linmidi",
"-orc",
"perc.saol",
"-sco",
"perc.sasl"};


#define APPNAME "sfront"
#define APPVERSION "0.87 10/15/04"
#define NSYS_NET 0
#define ARATE 44100.0F
#define ATIME 2.267574e-05F
#define KRATE 420.0F
#define KTIME 2.380952e-03F
#define KMTIME 2.380952e+00F
#define KUTIME 2381L
#define ACYCLE 105L
float tempo = 60.0F;
float scoremult = 2.380952e-03F;

#define ASYS_RENDER   0
#define ASYS_PLAYBACK 1
#define ASYS_TIMESYNC 2

#define ASYS_SHORT   0
#define ASYS_FLOAT   1

/* audio i/o */

#define ASYS_OCHAN 2L
#define ASYS_OTYPENAME  ASYS_SHORT
#define ASYS_OTYPE  short
long asys_osize = 0;
long obusidx = 0;

ASYS_OTYPE * asys_obuf = NULL;

#define ASYS_USERLATENCY  1
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

#define FLNOFF 0.002F

FILE * sfile = NULL;

#define BUS_output_bus 0
#define ENDBUS_output_bus 2
#define BUS_rvbus 2
#define ENDBUS 3
float bus[ENDBUS];

#define CSYS_EXTCHANSTART 0
int midimasterchannel = CSYS_EXTCHANSTART;

/* first 4144 locations for MIDI */
#define GBL_STARTVAR 4144
#define GBL_yaxurand 4144
#define GBL_kick 4145
#define GBL_snare 4146
#define GBL_lotom 4147
#define GBL_midtom 4148
#define GBL_hitom 4149
#define GBL_loconga 4150
#define GBL_midconga 4151
#define GBL_hiconga 4152
#define GBL_rimshot 4153
#define GBL_claves 4154
#define GBL_clap 4155
#define GBL_maracas 4156
#define GBL_cowbell 4157
#define GBL_ride 4158
#define GBL_crash 4159
#define GBL_hhopen 4160
#define GBL_hhclosed 4161
#define GBL_times 4162
#define GBL_inittimes 4163
#define GBL_midimap 4164
#define GBL_azimuth 4165
#define GBL_elevation 4166
#define GBL_levels 4167
#define GBL_string_qscale 4168
#define GBL_string_gscale 4169
#define GBL_string_vel 4170
#define GBL_string_poly 4171
#define GBL_yaxuinit 4172
#define GBL_yaxuenv 4173
#define GBL_exp1_return 4174
#define GBL_exp2_return 4175
#define GBL_ENDVAR 4176
/* global variables */

dstack global[GBL_ENDVAR+1];

#define TBL_GBL_yaxurand 0
#define TBL_GBL_kick 1
#define TBL_GBL_snare 2
#define TBL_GBL_lotom 3
#define TBL_GBL_midtom 4
#define TBL_GBL_hitom 5
#define TBL_GBL_loconga 6
#define TBL_GBL_midconga 7
#define TBL_GBL_hiconga 8
#define TBL_GBL_rimshot 9
#define TBL_GBL_claves 10
#define TBL_GBL_clap 11
#define TBL_GBL_maracas 12
#define TBL_GBL_cowbell 13
#define TBL_GBL_ride 14
#define TBL_GBL_crash 15
#define TBL_GBL_hhopen 16
#define TBL_GBL_hhclosed 17
#define TBL_GBL_times 18
#define TBL_GBL_midimap 19
#define TBL_GBL_azimuth 20
#define TBL_GBL_elevation 21
#define TBL_GBL_levels 22
#define TBL_GBL_string_qscale 23
#define TBL_GBL_string_gscale 24
#define TBL_GBL_string_vel 25
#define GBL_ENDTBL 26
struct tableinfo gtables[GBL_ENDTBL+1];

#define yaxupaxo_pitch 0
#define yaxupaxo_vel 1
#define yaxupaxo_ktime 2
#define yaxupaxo_stime 3
#define yaxupaxo_atime 4
#define yaxupaxo_rtime 5
#define yaxupaxo_yaxurand 6
#define yaxupaxo_y 7
#define yaxupaxo_attack 8
#define yaxupaxo_release 9
#define yaxupaxo_sustain 10
#define yaxupaxo__tvr0 11
#define yaxupaxo_kline1_first 12
#define yaxupaxo_kline1_addK 13
#define yaxupaxo_kline1_outT 14
#define yaxupaxo_kline1_mult 15
#define yaxupaxo_kline1_cdur 16
#define yaxupaxo_kline1_crp 17
#define yaxupaxo_kline1_clp 18
#define yaxupaxo_kline1_t 19
#define yaxupaxo_kline1_x2 20
#define yaxupaxo_kline1_dur1 21
#define yaxupaxo_kline1_x1 22
#define yaxupaxo_kline1_return 23
#define yaxupaxo_doscil2_play 24
#define yaxupaxo_doscil2_p 25
#define yaxupaxo_doscil2_t 26
#define yaxupaxo_doscil2_return 27
#define yaxupaxo_ENDVAR 28

#define string_kbd_pitch 0
#define string_kbd_velocity 1
#define string_kbd_string_vel 2
#define string_kbd_vval 3
#define string_kbd_kpitch 4
#define string_kbd__tvr1 5
#define string_kbd__tvr0 6
#define string_kbd_int1_return 7
#define string_kbd_tableread2_return 8
#define string_kbd_int3_return 9
#define string_kbd_tablewrite4_val 10
#define string_kbd_tablewrite4_index 11
#define string_kbd_tablewrite4_t 12
#define string_kbd_tablewrite4_return 13
#define string_kbd_int5_return 14
#define string_kbd_tablewrite6_val 15
#define string_kbd_tablewrite6_index 16
#define string_kbd_tablewrite6_t 17
#define string_kbd_tablewrite6_return 18
#define string_kbd_ENDVAR 19

#define string_audio_notenum 0
#define string_audio_a 1
#define string_audio_b 11
#define string_audio_g 21
#define string_audio_aa 31
#define string_audio_ab 32
#define string_audio_sg 33
#define string_audio_vw 34
#define string_audio_vwn 35
#define string_audio_nm 36
#define string_audio_ky 37
#define string_audio_silent 38
#define string_audio_out 39
#define string_audio_y 40
#define string_audio_y1 50
#define string_audio_y2 60
#define string_audio_sy 70
#define string_audio_ay 80
#define string_audio_ay1 81
#define string_audio_ay2 82
#define string_audio_dummy 83
#define string_audio_x 84
#define string_audio_string_resinit1_r 85
#define string_audio_string_resinit1_freq 95
#define string_audio_string_resinit1_q 105
#define string_audio_string_resinit1_j 115
#define string_audio_string_resinit1_scale 116
#define string_audio_string_resinit1_norm 117
#define string_audio_string_resinit1_string_qscale 118
#define string_audio_string_resinit1_string_gscale 119
#define string_audio_string_resinit1_return 120
#define string_audio_string_resinit1_int1_return 121
#define string_audio_string_resinit1_tableread2_return 122
#define string_audio_string_resinit1_cpsmidi3_return 123
#define string_audio_string_resinit1_int4_return 124
#define string_audio_string_resinit1_tableread5_return 125
#define string_audio_string_resinit1_exp6_return 126
#define string_audio_string_resinit1_cos7_return 127
#define string_audio_string_strikeinit2_ar 128
#define string_audio_string_strikeinit2_afreq 129
#define string_audio_string_strikeinit2_return 130
#define string_audio_string_strikeinit2_exp1_return 131
#define string_audio_rms3_buffer 132
#define string_audio_rms3_scale 133
#define string_audio_rms3_kcyc 134
#define string_audio_rms3_x 135
#define string_audio_rms3_return 136
#define string_audio_string_strikeupdate4_string_vel 137
#define string_audio_string_strikeupdate4_exit 138
#define string_audio_string_strikeupdate4_count 139
#define string_audio_string_strikeupdate4_return 140
#define string_audio_string_strikeupdate4_max1_x1 141
#define string_audio_string_strikeupdate4_max1_return 142
#define string_audio_string_strikeupdate4_int2_return 143
#define string_audio_string_strikeupdate4_tablewrite3_val 144
#define string_audio_string_strikeupdate4_tablewrite3_index 145
#define string_audio_string_strikeupdate4_tablewrite3_t 146
#define string_audio_string_strikeupdate4_tablewrite3_return 147
#define string_audio_string_strikeupdate4_int4_return 148
#define string_audio_string_strikeupdate4_tableread5_return 149
#define string_audio_string_strikeupdate4_int6_return 150
#define string_audio_string_strikeupdate4_tableread7_return 151
#define string_audio_string_strikeupdate4_int8_return 152
#define string_audio_string_strikeupdate4_tablewrite9_val 153
#define string_audio_string_strikeupdate4_tablewrite9_index 154
#define string_audio_string_strikeupdate4_tablewrite9_t 155
#define string_audio_string_strikeupdate4_tablewrite9_return 156
#define string_audio_arand5_return 157
#define string_audio_abs6_return 158
#define string_audio_ENDVAR 159

#define perc_pitch 0
#define perc_vel 1
#define perc_pmap 2
#define perc_scale 3
#define perc_az 4
#define perc_el 5
#define perc_eflag 6
#define perc_etime 7
#define perc_kpmap 8
#define perc_samp 9
#define perc_kick 10
#define perc_snare 11
#define perc_lotom 12
#define perc_midtom 13
#define perc_hitom 14
#define perc_loconga 15
#define perc_midconga 16
#define perc_hiconga 17
#define perc_rimshot 18
#define perc_claves 19
#define perc_clap 20
#define perc_maracas 21
#define perc_cowbell 22
#define perc_ride 23
#define perc_crash 24
#define perc_hhopen 25
#define perc_hhclosed 26
#define perc_midimap 27
#define perc_times 28
#define perc_azimuth 29
#define perc_elevation 30
#define perc_levels 31
#define perc__tvr1 32
#define perc__tvr0 33
#define perc_timeset1_kick 34
#define perc_timeset1_snare 35
#define perc_timeset1_lotom 36
#define perc_timeset1_midtom 37
#define perc_timeset1_hitom 38
#define perc_timeset1_loconga 39
#define perc_timeset1_midconga 40
#define perc_timeset1_hiconga 41
#define perc_timeset1_rimshot 42
#define perc_timeset1_claves 43
#define perc_timeset1_clap 44
#define perc_timeset1_maracas 45
#define perc_timeset1_cowbell 46
#define perc_timeset1_ride 47
#define perc_timeset1_crash 48
#define perc_timeset1_hhopen 49
#define perc_timeset1_hhclosed 50
#define perc_timeset1_times 51
#define perc_timeset1_return 52
#define perc_timeset1_ftlen1_return 53
#define perc_timeset1_tablewrite2_val 54
#define perc_timeset1_tablewrite2_index 55
#define perc_timeset1_tablewrite2_t 56
#define perc_timeset1_tablewrite2_return 57
#define perc_timeset1_ftlen3_return 58
#define perc_timeset1_tablewrite4_val 59
#define perc_timeset1_tablewrite4_index 60
#define perc_timeset1_tablewrite4_t 61
#define perc_timeset1_tablewrite4_return 62
#define perc_timeset1_ftlen5_return 63
#define perc_timeset1_tablewrite6_val 64
#define perc_timeset1_tablewrite6_index 65
#define perc_timeset1_tablewrite6_t 66
#define perc_timeset1_tablewrite6_return 67
#define perc_timeset1_ftlen7_return 68
#define perc_timeset1_tablewrite8_val 69
#define perc_timeset1_tablewrite8_index 70
#define perc_timeset1_tablewrite8_t 71
#define perc_timeset1_tablewrite8_return 72
#define perc_timeset1_ftlen9_return 73
#define perc_timeset1_tablewrite10_val 74
#define perc_timeset1_tablewrite10_index 75
#define perc_timeset1_tablewrite10_t 76
#define perc_timeset1_tablewrite10_return 77
#define perc_timeset1_ftlen11_return 78
#define perc_timeset1_tablewrite12_val 79
#define perc_timeset1_tablewrite12_index 80
#define perc_timeset1_tablewrite12_t 81
#define perc_timeset1_tablewrite12_return 82
#define perc_timeset1_ftlen13_return 83
#define perc_timeset1_tablewrite14_val 84
#define perc_timeset1_tablewrite14_index 85
#define perc_timeset1_tablewrite14_t 86
#define perc_timeset1_tablewrite14_return 87
#define perc_timeset1_ftlen15_return 88
#define perc_timeset1_tablewrite16_val 89
#define perc_timeset1_tablewrite16_index 90
#define perc_timeset1_tablewrite16_t 91
#define perc_timeset1_tablewrite16_return 92
#define perc_timeset1_ftlen17_return 93
#define perc_timeset1_tablewrite18_val 94
#define perc_timeset1_tablewrite18_index 95
#define perc_timeset1_tablewrite18_t 96
#define perc_timeset1_tablewrite18_return 97
#define perc_timeset1_ftlen19_return 98
#define perc_timeset1_tablewrite20_val 99
#define perc_timeset1_tablewrite20_index 100
#define perc_timeset1_tablewrite20_t 101
#define perc_timeset1_tablewrite20_return 102
#define perc_timeset1_ftlen21_return 103
#define perc_timeset1_tablewrite22_val 104
#define perc_timeset1_tablewrite22_index 105
#define perc_timeset1_tablewrite22_t 106
#define perc_timeset1_tablewrite22_return 107
#define perc_timeset1_ftlen23_return 108
#define perc_timeset1_tablewrite24_val 109
#define perc_timeset1_tablewrite24_index 110
#define perc_timeset1_tablewrite24_t 111
#define perc_timeset1_tablewrite24_return 112
#define perc_timeset1_ftlen25_return 113
#define perc_timeset1_tablewrite26_val 114
#define perc_timeset1_tablewrite26_index 115
#define perc_timeset1_tablewrite26_t 116
#define perc_timeset1_tablewrite26_return 117
#define perc_timeset1_ftlen27_return 118
#define perc_timeset1_tablewrite28_val 119
#define perc_timeset1_tablewrite28_index 120
#define perc_timeset1_tablewrite28_t 121
#define perc_timeset1_tablewrite28_return 122
#define perc_timeset1_ftlen29_return 123
#define perc_timeset1_tablewrite30_val 124
#define perc_timeset1_tablewrite30_index 125
#define perc_timeset1_tablewrite30_t 126
#define perc_timeset1_tablewrite30_return 127
#define perc_timeset1_ftlen31_return 128
#define perc_timeset1_tablewrite32_val 129
#define perc_timeset1_tablewrite32_index 130
#define perc_timeset1_tablewrite32_t 131
#define perc_timeset1_tablewrite32_return 132
#define perc_timeset1_ftlen33_return 133
#define perc_timeset1_tablewrite34_val 134
#define perc_timeset1_tablewrite34_index 135
#define perc_timeset1_tablewrite34_t 136
#define perc_timeset1_tablewrite34_return 137
#define perc_tableread2_index 138
#define perc_tableread2_t 139
#define perc_tableread2_return 140
#define perc_tableread3_index 141
#define perc_tableread3_t 142
#define perc_tableread3_return 143
#define perc_tableread4_index 144
#define perc_tableread4_t 145
#define perc_tableread4_return 146
#define perc_tableread5_index 147
#define perc_tableread5_t 148
#define perc_tableread5_return 149
#define perc_tableread6_index 150
#define perc_tableread6_t 151
#define perc_tableread6_return 152
#define perc_doscil7_play 153
#define perc_doscil7_p 154
#define perc_doscil7_t 155
#define perc_doscil7_return 156
#define perc_spatialize8_az_d1R 157
#define perc_spatialize8_az_d1L 158
#define perc_spatialize8_az_a1 159
#define perc_spatialize8_az_b1R 160
#define perc_spatialize8_az_b1L 161
#define perc_spatialize8_az_b0R 162
#define perc_spatialize8_az_b0L 163
#define perc_spatialize8_d7 164
#define perc_spatialize8_d6 165
#define perc_spatialize8_d5 166
#define perc_spatialize8_d4 167
#define perc_spatialize8_d3 168
#define perc_spatialize8_d2 169
#define perc_spatialize8_d1 170
#define perc_spatialize8_d0 171
#define perc_spatialize8_i6 172
#define perc_spatialize8_t6 173
#define perc_spatialize8_i5 174
#define perc_spatialize8_t5 175
#define perc_spatialize8_i4 176
#define perc_spatialize8_t4 177
#define perc_spatialize8_i3 178
#define perc_spatialize8_t3 179
#define perc_spatialize8_i2 180
#define perc_spatialize8_t2 181
#define perc_spatialize8_i1 182
#define perc_spatialize8_t1 183
#define perc_spatialize8_i0 184
#define perc_spatialize8_t0 185
#define perc_spatialize8_dis_d2 186
#define perc_spatialize8_dis_d1 187
#define perc_spatialize8_dis_a2 188
#define perc_spatialize8_dis_a1 189
#define perc_spatialize8_dis_b2 190
#define perc_spatialize8_dis_b1 191
#define perc_spatialize8_dis_b0 192
#define perc_spatialize8_odis 193
#define perc_spatialize8_oel 194
#define perc_spatialize8_oaz 195
#define perc_spatialize8_kcyc 196
#define perc_spatialize8_distance 197
#define perc_spatialize8_elevation 198
#define perc_spatialize8_azimuth 199
#define perc_spatialize8_x 200
#define perc_spatialize8_return 201
#define perc_ENDVAR 202

#define TBL_yaxupaxo_yaxurand 0
#define yaxupaxo_ENDTBL 1

#define TBL_string_kbd_string_vel 0
#define string_kbd_ENDTBL 1

#define TBL_string_audio_string_resinit1_string_qscale 0
#define TBL_string_audio_string_resinit1_string_gscale 1
#define TBL_string_audio_rms3_buffer 2
#define TBL_string_audio_string_strikeupdate4_string_vel 3
#define string_audio_ENDTBL 4

#define TBL_perc_kick 0
#define TBL_perc_snare 1
#define TBL_perc_lotom 2
#define TBL_perc_midtom 3
#define TBL_perc_hitom 4
#define TBL_perc_loconga 5
#define TBL_perc_midconga 6
#define TBL_perc_hiconga 7
#define TBL_perc_rimshot 8
#define TBL_perc_claves 9
#define TBL_perc_clap 10
#define TBL_perc_maracas 11
#define TBL_perc_cowbell 12
#define TBL_perc_ride 13
#define TBL_perc_crash 14
#define TBL_perc_hhopen 15
#define TBL_perc_hhclosed 16
#define TBL_perc_midimap 17
#define TBL_perc_times 18
#define TBL_perc_azimuth 19
#define TBL_perc_elevation 20
#define TBL_perc_levels 21
#define TBL_perc__tvr1 22
#define TBL_perc_timeset1_kick 23
#define TBL_perc_timeset1_snare 24
#define TBL_perc_timeset1_lotom 25
#define TBL_perc_timeset1_midtom 26
#define TBL_perc_timeset1_hitom 27
#define TBL_perc_timeset1_loconga 28
#define TBL_perc_timeset1_midconga 29
#define TBL_perc_timeset1_hiconga 30
#define TBL_perc_timeset1_rimshot 31
#define TBL_perc_timeset1_claves 32
#define TBL_perc_timeset1_clap 33
#define TBL_perc_timeset1_maracas 34
#define TBL_perc_timeset1_cowbell 35
#define TBL_perc_timeset1_ride 36
#define TBL_perc_timeset1_crash 37
#define TBL_perc_timeset1_hhopen 38
#define TBL_perc_timeset1_hhclosed 39
#define TBL_perc_timeset1_times 40
#define TBL_perc_spatialize8_d7 41
#define TBL_perc_spatialize8_d6 42
#define TBL_perc_spatialize8_d5 43
#define TBL_perc_spatialize8_d4 44
#define TBL_perc_spatialize8_d3 45
#define TBL_perc_spatialize8_d2 46
#define TBL_perc_spatialize8_d1 47
#define TBL_perc_spatialize8_d0 48
#define perc_ENDTBL 49

#define rv1_reverb1_a2_0 0
#define rv1_reverb1_a1_0 1
#define rv1_reverb1_b2_0 2
#define rv1_reverb1_b1_0 3
#define rv1_reverb1_b0_0 4
#define rv1_reverb1_d1_0 5
#define rv1_reverb1_d2_0 6
#define rv1_reverb1_g0_3 7
#define rv1_reverb1_g0_2 8
#define rv1_reverb1_g0_1 9
#define rv1_reverb1_g0_0 10
#define rv1_reverb1_dline0_3 11
#define rv1_reverb1_dline0_2 12
#define rv1_reverb1_dline0_1 13
#define rv1_reverb1_dline0_0 14
#define rv1_reverb1_ap2 15
#define rv1_reverb1_ap1 16
#define rv1_reverb1_f0 17
#define rv1_reverb1_x 18
#define rv1_reverb1_return 19
#define rv1_ENDVAR 20

#define TBL_rv1_reverb1_dline0_3 0
#define TBL_rv1_reverb1_dline0_2 1
#define TBL_rv1_reverb1_dline0_1 2
#define TBL_rv1_reverb1_dline0_0 3
#define TBL_rv1_reverb1_ap2 4
#define TBL_rv1_reverb1_ap1 5
#define rv1_ENDTBL 6

extern void sigint_handler(int);

float yaxupaxo__sym_kline1(struct ninstr_types *);
float yaxupaxo__sym_doscil2(struct ninstr_types *);
float string_kbd__sym_tablewrite4(struct ninstr_types *);
float string_kbd__sym_tablewrite6(struct ninstr_types *);
float string_audio__sym_string_resinit1(struct ninstr_types *);
float string_audio__sym_string_strikeinit2(struct ninstr_types *);
float string_audio__sym_rms3(struct ninstr_types *);
void string_audio__sym_rms3_spec(struct ninstr_types *);
float string_audio__sym_string_strikeupdate4(struct ninstr_types *);
float string_audio_string_strikeupdate4__sym_max1(struct ninstr_types *);
float string_audio_string_strikeupdate4__sym_tablewrite3(struct ninstr_types *);
float string_audio_string_strikeupdate4__sym_tablewrite9(struct ninstr_types *);
float perc__sym_timeset1(struct ninstr_types *);
float perc_timeset1__sym_tablewrite2(struct ninstr_types *);
float perc_timeset1__sym_tablewrite4(struct ninstr_types *);
float perc_timeset1__sym_tablewrite6(struct ninstr_types *);
float perc_timeset1__sym_tablewrite8(struct ninstr_types *);
float perc_timeset1__sym_tablewrite10(struct ninstr_types *);
float perc_timeset1__sym_tablewrite12(struct ninstr_types *);
float perc_timeset1__sym_tablewrite14(struct ninstr_types *);
float perc_timeset1__sym_tablewrite16(struct ninstr_types *);
float perc_timeset1__sym_tablewrite18(struct ninstr_types *);
float perc_timeset1__sym_tablewrite20(struct ninstr_types *);
float perc_timeset1__sym_tablewrite22(struct ninstr_types *);
float perc_timeset1__sym_tablewrite24(struct ninstr_types *);
float perc_timeset1__sym_tablewrite26(struct ninstr_types *);
float perc_timeset1__sym_tablewrite28(struct ninstr_types *);
float perc_timeset1__sym_tablewrite30(struct ninstr_types *);
float perc_timeset1__sym_tablewrite32(struct ninstr_types *);
float perc_timeset1__sym_tablewrite34(struct ninstr_types *);
float perc__sym_tableread2(struct ninstr_types *);
float perc__sym_tableread3(struct ninstr_types *);
float perc__sym_tableread4(struct ninstr_types *);
float perc__sym_tableread5(struct ninstr_types *);
float perc__sym_tableread6(struct ninstr_types *);
float perc__sym_doscil7(struct ninstr_types *);
float perc__sym_spatialize8(struct ninstr_types *);
float rv1__sym_reverb1(void);
void yaxupaxo_ipass(struct ninstr_types *);
void yaxupaxo_kpass(struct ninstr_types *);
void yaxupaxo_apass(struct ninstr_types *);
void string_kbd_ipass(struct ninstr_types *);
void string_kbd_kpass(struct ninstr_types *);
void string_audio_ipass(struct ninstr_types *);
void string_audio_kpass(struct ninstr_types *);
void string_audio_apass(struct ninstr_types *);
void perc_ipass(struct ninstr_types *);
void perc_kpass(struct ninstr_types *);
void perc_apass(struct ninstr_types *);

void rv1_ipass(void);
void rv1_kpass(void);
void rv1_apass(void);

float finput0(float);
float finGroup0(float);

extern float table_global_times[];
extern float table_global_midimap[];
extern float table_global_azimuth[];
extern float table_global_elevation[];
extern float table_global_levels[];
extern float table_global_string_qscale[];
extern float table_global_string_gscale[];
extern float table_global_string_vel[];

#define CSYS_GIVENENDTIME 1

#define MAXENDTIME 1E+37

float endtime = 216000.0F;
#define MAXCNOTES 256

#define CSYS_INSTRNUM 5

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
   epr(0,NULL,"linmidi control driver", message);
}

#define CSYS_GLOBALNUM 30

csys_varstruct csys_global[CSYS_GLOBALNUM] = {
0, "yaxurand", 3, CSYS_TABLE, CSYS_NORMAL, 1, 
1, "kick", 5, CSYS_TABLE, CSYS_NORMAL, 1, 
2, "snare", 7, CSYS_TABLE, CSYS_NORMAL, 1, 
3, "lotom", 8, CSYS_TABLE, CSYS_NORMAL, 1, 
4, "midtom", 9, CSYS_TABLE, CSYS_NORMAL, 1, 
5, "hitom", 10, CSYS_TABLE, CSYS_NORMAL, 1, 
6, "loconga", 11, CSYS_TABLE, CSYS_NORMAL, 1, 
7, "midconga", 12, CSYS_TABLE, CSYS_NORMAL, 1, 
8, "hiconga", 13, CSYS_TABLE, CSYS_NORMAL, 1, 
9, "rimshot", 14, CSYS_TABLE, CSYS_NORMAL, 1, 
10, "claves", 15, CSYS_TABLE, CSYS_NORMAL, 1, 
11, "clap", 16, CSYS_TABLE, CSYS_NORMAL, 1, 
12, "maracas", 17, CSYS_TABLE, CSYS_NORMAL, 1, 
13, "cowbell", 18, CSYS_TABLE, CSYS_NORMAL, 1, 
14, "ride", 19, CSYS_TABLE, CSYS_NORMAL, 1, 
15, "crash", 20, CSYS_TABLE, CSYS_NORMAL, 1, 
16, "hhopen", 21, CSYS_TABLE, CSYS_NORMAL, 1, 
17, "hhclosed", 22, CSYS_TABLE, CSYS_NORMAL, 1, 
18, "times", 23, CSYS_TABLE, CSYS_NORMAL, 1, 
4163, "inittimes", 25, CSYS_KRATE, CSYS_NORMAL, 1, 
19, "midimap", 26, CSYS_TABLE, CSYS_NORMAL, 1, 
20, "azimuth", 28, CSYS_TABLE, CSYS_NORMAL, 1, 
21, "elevation", 29, CSYS_TABLE, CSYS_NORMAL, 1, 
22, "levels", 30, CSYS_TABLE, CSYS_NORMAL, 1, 
23, "string_qscale", 31, CSYS_TABLE, CSYS_NORMAL, 1, 
24, "string_gscale", 34, CSYS_TABLE, CSYS_NORMAL, 1, 
25, "string_vel", 35, CSYS_TABLE, CSYS_NORMAL, 1, 
4171, "string_poly", 37, CSYS_KRATE, CSYS_NORMAL, 1, 
4172, "yaxuinit", 40, CSYS_KRATE, CSYS_NORMAL, 1, 
4173, "yaxuenv", 41, CSYS_KRATE, CSYS_NORMAL, 1 };

#define CSYS_TARGETNUM 0

csys_targetstruct csys_target[1];

#define CSYS_NOLABEL 0

#define CSYS_LABELNUM 0

csys_labelstruct csys_labels[1];

#define CSYS_PRESETNUM 3

csys_presetstruct csys_presets[CSYS_PRESETNUM] = {
3, 0 ,
1, 1 ,
0, 2 };

#define CSYS_SAMPLENUM 0

csys_samplestruct csys_samples[1];

#define CSYS_BUSNUM 2

csys_busstruct csys_bus[CSYS_BUSNUM] = {
0, "output_bus",2, 0 ,
1, "rvbus",1, 2 };

#define CSYS_ROUTENUM 0

csys_routestruct csys_route[1];

#define CSYS_SEND_BNUM_0 1

int csys_send0_bus[CSYS_SEND_BNUM_0] = {
 1};

#define CSYS_SENDNUM 1

csys_sendstruct csys_send[CSYS_SENDNUM] = {
4, CSYS_SEND_BNUM_0, &(csys_send0_bus[0]) };

#define CSYS_yaxupaxo_VARNUM 13

csys_varstruct csys_yaxupaxo_vars[CSYS_yaxupaxo_VARNUM] = {
0, "pitch", 46, CSYS_IRATE, CSYS_PFIELD, 1 ,
0, "vel", 47, CSYS_IRATE, CSYS_PFIELD, 1 ,
1, "ktime", 158, CSYS_IRATE, CSYS_NORMAL, 1 ,
1, "stime", 159, CSYS_IRATE, CSYS_NORMAL, 1 ,
1, "atime", 160, CSYS_IRATE, CSYS_NORMAL, 1 ,
1, "rtime", 161, CSYS_IRATE, CSYS_NORMAL, 1 ,
0, "yaxurand", 3, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "y", 88, CSYS_ARATE, CSYS_NORMAL, 1 ,
1, "attack", 162, CSYS_IRATE, CSYS_NORMAL, 1 ,
1, "release", 163, CSYS_IRATE, CSYS_NORMAL, 1 ,
1, "sustain", 164, CSYS_IRATE, CSYS_NORMAL, 1 ,
-1, "yaxuinit", 40, CSYS_KRATE, CSYS_IMPORTEXPORT, 1 ,
-1, "yaxuenv", 41, CSYS_KRATE, CSYS_IMPORTEXPORT, 1  };

#define CSYS_string_kbd_VARNUM 6

csys_varstruct csys_string_kbd_vars[CSYS_string_kbd_VARNUM] = {
0, "pitch", 46, CSYS_IRATE, CSYS_PFIELD, 1 ,
0, "velocity", 154, CSYS_IRATE, CSYS_PFIELD, 1 ,
0, "string_vel", 35, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
-1, "string_poly", 37, CSYS_KRATE, CSYS_IMPORTEXPORT, 1 ,
0, "vval", 155, CSYS_KRATE, CSYS_NORMAL, 1 ,
0, "kpitch", 156, CSYS_KRATE, CSYS_NORMAL, 1  };

#define CSYS_string_audio_VARNUM 22

csys_varstruct csys_string_audio_vars[CSYS_string_audio_VARNUM] = {
0, "notenum", 122, CSYS_IRATE, CSYS_PFIELD, 1 ,
0, "a", 68, CSYS_IRATE, CSYS_NORMAL, 10 ,
0, "b", 70, CSYS_IRATE, CSYS_NORMAL, 10 ,
0, "g", 72, CSYS_IRATE, CSYS_NORMAL, 10 ,
0, "aa", 131, CSYS_IRATE, CSYS_NORMAL, 1 ,
0, "ab", 132, CSYS_IRATE, CSYS_NORMAL, 1 ,
0, "sg", 133, CSYS_IRATE, CSYS_NORMAL, 1 ,
0, "vw", 134, CSYS_IRATE, CSYS_NORMAL, 1 ,
0, "vwn", 135, CSYS_IRATE, CSYS_NORMAL, 1 ,
0, "nm", 140, CSYS_KRATE, CSYS_NORMAL, 1 ,
0, "ky", 139, CSYS_KRATE, CSYS_NORMAL, 1 ,
0, "silent", 141, CSYS_KRATE, CSYS_NORMAL, 1 ,
0, "out", 62, CSYS_ARATE, CSYS_NORMAL, 1 ,
0, "y", 88, CSYS_ARATE, CSYS_NORMAL, 10 ,
0, "y1", 144, CSYS_ARATE, CSYS_NORMAL, 10 ,
0, "y2", 145, CSYS_ARATE, CSYS_NORMAL, 10 ,
0, "sy", 146, CSYS_ARATE, CSYS_NORMAL, 10 ,
0, "ay", 147, CSYS_ARATE, CSYS_NORMAL, 1 ,
0, "ay1", 148, CSYS_ARATE, CSYS_NORMAL, 1 ,
0, "ay2", 149, CSYS_ARATE, CSYS_NORMAL, 1 ,
1, "dummy", 150, CSYS_ARATE, CSYS_NORMAL, 1 ,
0, "x", 87, CSYS_ARATE, CSYS_NORMAL, 1  };

#define CSYS_perc_VARNUM 33

csys_varstruct csys_perc_vars[CSYS_perc_VARNUM] = {
0, "pitch", 46, CSYS_IRATE, CSYS_PFIELD, 1 ,
0, "vel", 47, CSYS_IRATE, CSYS_PFIELD, 1 ,
0, "pmap", 48, CSYS_IRATE, CSYS_NORMAL, 1 ,
0, "scale", 49, CSYS_IRATE, CSYS_NORMAL, 1 ,
0, "az", 50, CSYS_IRATE, CSYS_NORMAL, 1 ,
0, "el", 51, CSYS_IRATE, CSYS_NORMAL, 1 ,
0, "eflag", 52, CSYS_KRATE, CSYS_NORMAL, 1 ,
0, "etime", 53, CSYS_KRATE, CSYS_NORMAL, 1 ,
0, "kpmap", 54, CSYS_KRATE, CSYS_NORMAL, 1 ,
0, "samp", 55, CSYS_ARATE, CSYS_NORMAL, 1 ,
0, "kick", 5, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "snare", 7, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "lotom", 8, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "midtom", 9, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "hitom", 10, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "loconga", 11, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "midconga", 12, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "hiconga", 13, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "rimshot", 14, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "claves", 15, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "clap", 16, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "maracas", 17, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "cowbell", 18, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "ride", 19, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "crash", 20, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "hhopen", 21, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "hhclosed", 22, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "midimap", 26, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "times", 23, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "azimuth", 28, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "elevation", 29, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
0, "levels", 30, CSYS_TABLE, CSYS_IMPORTEXPORT, 1 ,
-1, "inittimes", 25, CSYS_KRATE, CSYS_IMPORTEXPORT, 1  };

#define CSYS_rv_VARNUM 0

csys_varstruct csys_rv_vars[1];

csys_instrstruct csys_instr[CSYS_INSTRNUM] = {
0, "yaxupaxo",157, CSYS_yaxupaxo_VARNUM, &(csys_yaxupaxo_vars[0]),1, 0 ,
1, "string_kbd",38, CSYS_string_kbd_VARNUM, &(csys_string_kbd_vars[0]),2, 0 ,
2, "string_audio",39, CSYS_string_audio_VARNUM, &(csys_string_audio_vars[0]),1, 8 ,
3, "perc",0, CSYS_perc_VARNUM, &(csys_perc_vars[0]),2, 0 ,
4, "rv",1, CSYS_rv_VARNUM, &(csys_rv_vars[0]),1, 1 };

struct instr_line cm_yaxupaxo[MAXCNOTES];
struct instr_line * cm_yaxupaxo___first = &cm_yaxupaxo[1];
struct instr_line * cm_yaxupaxo___last = &cm_yaxupaxo[0];
struct instr_line * cm_yaxupaxo___end = &cm_yaxupaxo[MAXCNOTES-1];
struct instr_line * cm_yaxupaxo___next = NULL;

struct instr_line cm_string_kbd[MAXCNOTES];
struct instr_line * cm_string_kbd___first = &cm_string_kbd[1];
struct instr_line * cm_string_kbd___last = &cm_string_kbd[0];
struct instr_line * cm_string_kbd___end = &cm_string_kbd[MAXCNOTES-1];
struct instr_line * cm_string_kbd___next = NULL;

struct instr_line cm_perc[MAXCNOTES];
struct instr_line * cm_perc___first = &cm_perc[1];
struct instr_line * cm_perc___last = &cm_perc[0];
struct instr_line * cm_perc___end = &cm_perc[MAXCNOTES-1];
struct instr_line * cm_perc___next = NULL;

struct instr_line e_rv[1];

#define CSYS_CCPOS 0
#define CSYS_TOUCHPOS 128
#define CSYS_CHTOUCHPOS 256
#define CSYS_BENDPOS 257
#define CSYS_EXTPOS 258
#define CSYS_FRAMELEN 259
#define CSYS_MAXPRESETS 3

#define CSYS_NULLPROGRAM 0

struct instr_line **cmp_first[CSYS_MAXPRESETS] = {
&cm_perc___first,&cm_string_kbd___first,&cm_yaxupaxo___first};

struct instr_line **cmp_last[CSYS_MAXPRESETS] = {
&cm_perc___last,&cm_string_kbd___last,&cm_yaxupaxo___last};

struct instr_line **cmp_end[CSYS_MAXPRESETS] = {
&cm_perc___end,&cm_string_kbd___end,&cm_yaxupaxo___end};

struct instr_line **cmp_next[CSYS_MAXPRESETS] = {
&cm_perc___next,&cm_string_kbd___next,&cm_yaxupaxo___next};

#define CSYS_MAXCINCHAN 16

#define CSYS_MAXEXTCHAN 16

struct instr_line **cme_first[CSYS_MAXEXTCHAN] = {
&cm_perc___first,&cm_string_kbd___first,&cm_yaxupaxo___first,&cm_perc___first,&cm_string_kbd___first,
&cm_yaxupaxo___first,&cm_perc___first,&cm_string_kbd___first,&cm_yaxupaxo___first,&cm_perc___first,
&cm_string_kbd___first,&cm_yaxupaxo___first,&cm_perc___first,&cm_string_kbd___first,&cm_yaxupaxo___first,
&cm_perc___first};

struct instr_line **cme_last[CSYS_MAXEXTCHAN] = {
&cm_perc___last,&cm_string_kbd___last,&cm_yaxupaxo___last,&cm_perc___last,&cm_string_kbd___last,
&cm_yaxupaxo___last,&cm_perc___last,&cm_string_kbd___last,&cm_yaxupaxo___last,&cm_perc___last,
&cm_string_kbd___last,&cm_yaxupaxo___last,&cm_perc___last,&cm_string_kbd___last,&cm_yaxupaxo___last,
&cm_perc___last};

struct instr_line **cme_end[CSYS_MAXEXTCHAN] = {
&cm_perc___end,&cm_string_kbd___end,&cm_yaxupaxo___end,&cm_perc___end,&cm_string_kbd___end,
&cm_yaxupaxo___end,&cm_perc___end,&cm_string_kbd___end,&cm_yaxupaxo___end,&cm_perc___end,
&cm_string_kbd___end,&cm_yaxupaxo___end,&cm_perc___end,&cm_string_kbd___end,&cm_yaxupaxo___end,
&cm_perc___end};

struct instr_line **cme_next[CSYS_MAXEXTCHAN] = {
&cm_perc___next,&cm_string_kbd___next,&cm_yaxupaxo___next,&cm_perc___next,&cm_string_kbd___next,
&cm_yaxupaxo___next,&cm_perc___next,&cm_string_kbd___next,&cm_yaxupaxo___next,&cm_perc___next,
&cm_string_kbd___next,&cm_yaxupaxo___next,&cm_perc___next,&cm_string_kbd___next,&cm_yaxupaxo___next,
&cm_perc___next};

int cme_preset[CSYS_MAXEXTCHAN] = {
0,1,2,CSYS_MAXPRESETS,CSYS_MAXPRESETS,
CSYS_MAXPRESETS,CSYS_MAXPRESETS,CSYS_MAXPRESETS,CSYS_MAXPRESETS,CSYS_MAXPRESETS,
CSYS_MAXPRESETS,CSYS_MAXPRESETS,CSYS_MAXPRESETS,CSYS_MAXPRESETS,CSYS_MAXPRESETS,
CSYS_MAXPRESETS};

int csys_bank = 0;
int csys_banklsb = 0;
int csys_bankmsb = 0;

#define MAXLINES 256
#define MAXSTATE 264

struct instr_line d_string_audio[MAXLINES];
struct instr_line * d_string_audio___first = &d_string_audio[1];
struct instr_line * d_string_audio___last = &d_string_audio[0];
struct instr_line * d_string_audio___end = &d_string_audio[MAXLINES-1];
struct instr_line * d_string_audio___next = NULL;

#define MAXVARSTATE 202
#define MAXTABLESTATE 49

/* ninstr: used for score, effects, */
/* and dynamic instruments          */

struct ninstr_types {

struct instr_line * iline; /* pointer to score line */
dstack v[MAXVARSTATE];     /* parameters & variables*/
struct tableinfo t[MAXTABLESTATE]; /* tables */

} ninstr[MAXSTATE];

#define CSYS_CDRIVER_LINMIDI
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

#define CSYS_MIDI_SPECIAL  0x70u
#define CSYS_MIDI_NOOP     0x70u
#define CSYS_MIDI_NEWTEMPO 0x71u
#define CSYS_MIDI_ENDTIME  0x72u
#define CSYS_MIDI_POWERUP  0x73u
#define CSYS_MIDI_TSTART   0x7Du
#define CSYS_MIDI_MANUEX   0x7Du
#define CSYS_MIDI_MVOLUME  0x7Eu
#define CSYS_MIDI_GMRESET  0x7Fu
#define CSYS_MIDI_NOTEOFF  0x80u
#define CSYS_MIDI_NOTEON   0x90u
#define CSYS_MIDI_PTOUCH   0xA0u
#define CSYS_MIDI_CC       0xB0u
#define CSYS_MIDI_PROGRAM  0xC0u
#define CSYS_MIDI_CTOUCH   0xD0u
#define CSYS_MIDI_WHEEL    0xE0u
#define CSYS_MIDI_SYSTEM   0xF0u

#define CSYS_MIDI_SYSTEM_SYSEX_START  0xF0u
#define CSYS_MIDI_SYSTEM_QFRAME       0xF1u
#define CSYS_MIDI_SYSTEM_SONG_PP      0xF2u
#define CSYS_MIDI_SYSTEM_SONG_SELECT  0xF3u
#define CSYS_MIDI_SYSTEM_UNUSED1      0xF4u
#define CSYS_MIDI_SYSTEM_UNUSED2      0xF5u
#define CSYS_MIDI_SYSTEM_TUNE_REQUEST 0xF6u
#define CSYS_MIDI_SYSTEM_SYSEX_END    0xF7u
#define CSYS_MIDI_SYSTEM_CLOCK        0xF8u
#define CSYS_MIDI_SYSTEM_TICK         0xF9u
#define CSYS_MIDI_SYSTEM_START        0xFAu
#define CSYS_MIDI_SYSTEM_CONTINUE     0xFBu
#define CSYS_MIDI_SYSTEM_STOP         0xFCu
#define CSYS_MIDI_SYSTEM_UNUSED3      0xFDu
#define CSYS_MIDI_SYSTEM_SENSE        0xFEu
#define CSYS_MIDI_SYSTEM_RESET        0xFFu

#define CSYS_MIDI_CC_BANKSELECT_MSB  0x00u
#define CSYS_MIDI_CC_MODWHEEL_MSB    0x01u
#define CSYS_MIDI_CC_BREATHCNTRL_MSB 0x02u
#define CSYS_MIDI_CC_FOOTCNTRL_MSB   0x04u
#define CSYS_MIDI_CC_PORTAMENTO_MSB  0x05u
#define CSYS_MIDI_CC_DATAENTRY_MSB   0x06u
#define CSYS_MIDI_CC_CHANVOLUME_MSB  0x07u
#define CSYS_MIDI_CC_BALANCE_MSB     0x08u
#define CSYS_MIDI_CC_PAN_MSB         0x0Au
#define CSYS_MIDI_CC_EXPRESSION_MSB  0x0Bu
#define CSYS_MIDI_CC_EFFECT1_MSB     0x0Cu
#define CSYS_MIDI_CC_EFFECT2_MSB     0x0Du
#define CSYS_MIDI_CC_GEN1_MSB        0x10u
#define CSYS_MIDI_CC_GEN2_MSB        0x11u
#define CSYS_MIDI_CC_GEN3_MSB        0x12u
#define CSYS_MIDI_CC_GEN4_MSB        0x13u
#define CSYS_MIDI_CC_BANKSELECT_LSB  0x20u
#define CSYS_MIDI_CC_MODWHEEL_LSB    0x21u
#define CSYS_MIDI_CC_BREATHCNTRL_LSB 0x22u
#define CSYS_MIDI_CC_FOOTCNTRL_LSB   0x24u
#define CSYS_MIDI_CC_PORTAMENTO_LSB  0x25u
#define CSYS_MIDI_CC_DATAENTRY_LSB   0x26u
#define CSYS_MIDI_CC_CHANVOLUME_LSB  0x27u
#define CSYS_MIDI_CC_BALANCE_LSB     0x28u
#define CSYS_MIDI_CC_PAN_LSB         0x2Au
#define CSYS_MIDI_CC_EXPRESSION_LSB  0x2Bu
#define CSYS_MIDI_CC_EFFECT1_LSB     0x2Cu
#define CSYS_MIDI_CC_EFFECT2_LSB     0x2Du
#define CSYS_MIDI_CC_GEN1_LSB        0x30u
#define CSYS_MIDI_CC_GEN2_LSB        0x31u
#define CSYS_MIDI_CC_GEN3_LSB        0x32u
#define CSYS_MIDI_CC_GEN4_LSB        0x33u
#define CSYS_MIDI_CC_SUSTAIN         0x40u
#define CSYS_MIDI_CC_PORTAMENTO      0x41u
#define CSYS_MIDI_CC_SUSTENUTO       0x42u
#define CSYS_MIDI_CC_SOFTPEDAL       0x43u
#define CSYS_MIDI_CC_LEGATO          0x44u
#define CSYS_MIDI_CC_HOLD2           0x45u
#define CSYS_MIDI_CC_SOUNDCONTROL1   0x46u
#define CSYS_MIDI_CC_SOUNDCONTROL2   0x47u
#define CSYS_MIDI_CC_SOUNDCONTROL3   0x48u
#define CSYS_MIDI_CC_SOUNDCONTROL4   0x49u
#define CSYS_MIDI_CC_SOUNDCONTROL5   0x4Au
#define CSYS_MIDI_CC_SOUNDCONTROL6   0x4Bu
#define CSYS_MIDI_CC_SOUNDCONTROL7   0x4Cu
#define CSYS_MIDI_CC_SOUNDCONTROL8   0x4Du
#define CSYS_MIDI_CC_SOUNDCONTROL9   0x4Eu
#define CSYS_MIDI_CC_SOUNDCONTROL10  0x4Fu
#define CSYS_MIDI_CC_GEN5            0x50u
#define CSYS_MIDI_CC_GEN6            0x51u
#define CSYS_MIDI_CC_GEN7            0x52u
#define CSYS_MIDI_CC_GEN8            0x53u
#define CSYS_MIDI_CC_PORTAMENTOSRC   0x54u
#define CSYS_MIDI_CC_EFFECT1DEPTH    0x5Bu
#define CSYS_MIDI_CC_EFFECT2DEPTH    0x5Cu
#define CSYS_MIDI_CC_EFFECT3DEPTH    0x5Du
#define CSYS_MIDI_CC_EFFECT4DEPTH    0x5Eu
#define CSYS_MIDI_CC_EFFECT5DEPTH    0x5Fu
#define CSYS_MIDI_CC_DATAENTRYPLUS   0x60u
#define CSYS_MIDI_CC_DATAENTRYMINUS  0x61u
#define CSYS_MIDI_CC_NRPN_LSB        0x62u
#define CSYS_MIDI_CC_NRPN_MSB        0x63u
#define CSYS_MIDI_CC_RPN_LSB         0x64u
#define CSYS_MIDI_CC_RPN_MSB         0x65u
#define CSYS_MIDI_CC_ALLSOUNDOFF     0x78u
#define CSYS_MIDI_CC_RESETALLCONTROL 0x79u
#define CSYS_MIDI_CC_LOCALCONTROL    0x7Au
#define CSYS_MIDI_CC_ALLNOTESOFF     0x7Bu
#define CSYS_MIDI_CC_OMNI_OFF        0x7Cu
#define CSYS_MIDI_CC_OMNI_ON         0x7Du
#define CSYS_MIDI_CC_MONOMODE        0x7Eu
#define CSYS_MIDI_CC_POLYMODE        0x7Fu


/*
#    Sfront, a SAOL to C translator    
#    This file: Merged linux/freebsd MIDI Input control driver 
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


#include <fcntl.h>
#include <signal.h>  
#include <sys/time.h>  

/****************************************************************/
/****************************************************************/
/*               MIDI Input control driver for sfront           */ 
/****************************************************************/

#ifndef CSYSI_MIDIDEV
#define CSYSI_MIDIDEV "/dev/midi00"
#endif

#define CSYSI_BUFFSIZE    1024
#define CSYSI_SYSEX_EOX   0xF7

/* set CSYSI_DELAY to 0 to wait for partially completed MIDI commands */
/* waiting for commands decreases variance of the latency, at the     */
/* expense of losing computation cycles                               */

#define CSYSI_DELAY 1

/* variables for SIGALRM for MIDI overrun */

/* period for interrupt: 320us per MIDI byte @ 128 bytes, minus safety zone */

#define CSYSI_ALARMPERIOD  40000

/* maximum number of I/O retries before termination */

#define CSYSI_MAXRETRY 256

sigset_t         csysi_overrun_mask;    /* for masking off overrun interrupt */
struct sigaction csysi_overrun_action;  /* for setting up overrun interrupt  */
struct itimerval csysi_overrun_timer;   /* for setting up overrun timer      */

/* flag for new note on/off */

int csysi_newnote = 0;

/* MIDI parsing state variables */

int csysi_midi = 0;

unsigned char csysi_hold[CSYSI_BUFFSIZE];
int csysi_holdidx = 0;

unsigned char csysi_data[CSYSI_BUFFSIZE];

long csysi_len;
long csysi_cnt;
unsigned char csysi_cmd;
unsigned char csysi_num;
unsigned short csysi_extchan;
unsigned char csysi_ndata = 0xFF;

/****************************************************************/
/*          generic error-checking wrappers                     */
/****************************************************************/

#define  CSYSI_ERROR_RETURN(x) do {\
      fprintf(stderr, "  Error: %s.\n", x);\
      fprintf(stderr, "  Errno Message: %s\n\n", strerror(errno));\
      return CSYS_ERROR; } while (0)

#define  CSYSI_ERROR_TERMINATE(x) do {\
      fprintf(stderr, "  Runtime Errno Message: %s\n", strerror(errno));\
      epr(0,NULL,NULL, "Soundcard error -- " x );}  while (0)


/****************************************************************/
/*         signal handler to catch MIDI buffer overruns         */
/****************************************************************/

void csysi_overrun_handler(int signum) 

{   
  int retry = 0;
  int len;

  while ((len = read(csysi_midi, &(csysi_hold[csysi_holdidx]), 
	     CSYSI_BUFFSIZE-csysi_holdidx)) < 0)
    {
      if (++retry > CSYSI_MAXRETRY)
	CSYSI_ERROR_TERMINATE("Too many I/O retries -- csysi_overrun_handler");

      if (errno == EAGAIN)      /* no data ready */
	break;
      if (errno == EINTR)       /* interrupted, try again */
	continue;

      CSYSI_ERROR_TERMINATE("Couldn't read MIDI device");
    }

  if (len > 0)
    {
      if ((csysi_holdidx += len) >= CSYSI_BUFFSIZE)
	fprintf(stderr, "  Warning: MIDI overrun, data lost\n\n");
    }

  /* reset timer */

  if (setitimer(ITIMER_REAL, &csysi_overrun_timer, NULL) < 0)
    CSYSI_ERROR_TERMINATE("Couldn't reset ITIMER_REAL timer");
}


/****************************************************************/
/*             initialization routine for control               */
/****************************************************************/

int csys_setup(void)
     
{

  csysi_midi = open(CSYSI_MIDIDEV, O_RDONLY|O_NONBLOCK);

  if (csysi_midi == -1)
    CSYSI_ERROR_RETURN("Can't open MIDI input device");

  /* set up mask for overrun timer */
  
  if (sigemptyset(&csysi_overrun_mask) < 0)
    CSYSI_ERROR_RETURN("Couldn't run sigemptyset(overrun) OS call");

  if (sigaddset(&csysi_overrun_mask, SIGALRM) < 0)
    CSYSI_ERROR_RETURN("Couldn't run sigaddset(overrun) OS call");

  /* set up signal handler for overrun timer */
  
  if (sigemptyset(&csysi_overrun_action.sa_mask) < 0)
    CSYSI_ERROR_RETURN("Couldn't run sigemptyset(oaction) OS call");

  csysi_overrun_action.sa_flags = SA_RESTART;
  csysi_overrun_action.sa_handler = csysi_overrun_handler;
  
  if (sigaction(SIGALRM, &csysi_overrun_action, NULL) < 0)
    CSYSI_ERROR_RETURN("Couldn't set up SIGALRM signal handler");

  /* set up timer and arm */

  csysi_overrun_timer.it_value.tv_sec = 0;
  csysi_overrun_timer.it_value.tv_usec = CSYSI_ALARMPERIOD;
  csysi_overrun_timer.it_interval.tv_sec = 0;
  csysi_overrun_timer.it_interval.tv_usec = 0;

  if (setitimer(ITIMER_REAL, &csysi_overrun_timer, NULL) < 0)
    CSYSI_ERROR_RETURN("Couldn't set up ITIMER_REAL timer");

  return CSYS_DONE;
}

/****************************************************************/
/*       unmasks overrun timer at end of MIDI parsing           */
/****************************************************************/

int csysi_midiparseover(void)

{
  if (sigprocmask(SIG_UNBLOCK, &csysi_overrun_mask, NULL) < 0)
    CSYSI_ERROR_TERMINATE("Couldn't unmask MIDI overrun timer");

  return CSYS_NONE;
}

/****************************************************************/
/*             polling routine for new data                     */
/****************************************************************/

int csys_newdata(void)
     
{
  int i;
  int retry = 0;
  int len;

  /* block overrun time and reset it */

  if (sigprocmask(SIG_BLOCK, &csysi_overrun_mask, NULL) < 0)
    CSYSI_ERROR_TERMINATE("Couldn't mask MIDI overrun timer");

  if (setitimer(ITIMER_REAL, &csysi_overrun_timer, NULL) < 0)
    CSYSI_ERROR_TERMINATE("Couldn't reset ITIMER_REAL timer");

  if (!csysi_holdidx)
    {
      while ((len = read(csysi_midi, csysi_hold, CSYSI_BUFFSIZE)) < 0)
	{      
	  if (++retry > CSYSI_MAXRETRY)
	    CSYSI_ERROR_TERMINATE("Too many I/O retries -- csys_newdata(if)");

	  if (errno == EAGAIN)
	    return csysi_midiparseover();   /* no data ready, so leave */
	  if (errno == EINTR)
	    continue;                       /* interrupted, try again */

	  /* all other errors fatal */

	  CSYSI_ERROR_TERMINATE("Couldn't read MIDI device");
	}
    }
  else
    {
      while ((len = read(csysi_midi, &(csysi_hold[csysi_holdidx]), 
	       CSYSI_BUFFSIZE-csysi_holdidx)) < 0)
	{
	  if (++retry > CSYSI_MAXRETRY)
	    CSYSI_ERROR_TERMINATE("Too many I/O retries -- csys_newdata(el)");

	  if (errno == EAGAIN)
	    break;                      /* no data ready, process buffer */
	  if (errno == EINTR)
	    continue;                   /* interrupted, try again */

	  /* all other errors fatal */

	  CSYSI_ERROR_TERMINATE("Couldn't read MIDI device");
	}

      len = (len < 0) ? csysi_holdidx : len + csysi_holdidx;
      csysi_holdidx = 0;
    }

  csysi_newnote = csysi_len = csysi_cnt = 0;

  for (i = 0; i < len; i++)
    if (csysi_hold[i] <= CSYSI_SYSEX_EOX)
      csysi_data[csysi_len++] = csysi_hold[i];

  if (!csysi_len) 
    return csysi_midiparseover();

  /* leave interrupts locked until all data transferred */

  return CSYS_MIDIEVENTS;
  
}

/****************************************************************/
/*             gets one byte from MIDI stream                   */
/****************************************************************/

unsigned char csysi_getbyte(void)

{
  unsigned char d;
  int retry = 0;

  /* used when we need to risk waiting for the next byte */

  while (1)
    {
      if (read(csysi_midi, &d, 1) != 1)
	{
	  if (errno == EAGAIN) /* no data ready  */
	    {
	      retry = 0;
	      continue;
	    }
	  if (errno == EINTR) /* interrupted */
	    {	  
	      if (++retry > CSYSI_MAXRETRY)
		CSYSI_ERROR_TERMINATE("Too many I/O retries -- csysi_getbyte");
	      continue;
	    }
	  CSYSI_ERROR_TERMINATE("Couldn't read MIDI device");
	}
      else
	{
	  retry = 0;
	  if (d <= CSYSI_SYSEX_EOX)
	    break;
	  else
	    continue;
	}
    }

  return d;

}

/****************************************************************/
/*             flushes MIDI system messages                     */
/****************************************************************/

int csysi_sysflush(unsigned short type)

{
  unsigned char byte;

  if ((type == 6) || /* one-byte messages */
      (type == 1) || /* undefined messages */
      (type == 4) ||
      (type == 5))
    { 
      if (csysi_cnt == csysi_len)
	return csysi_midiparseover();
      else
	return CSYS_MIDIEVENTS;
    }
  
  if (type == 3) /* song select -- 1 data byte */
    {
      if (csysi_cnt == csysi_len)
	csysi_getbyte();
      else
	csysi_cnt++;
      if (csysi_cnt == csysi_len)
	return csysi_midiparseover();
      else
	return CSYS_MIDIEVENTS;
    }
  
  if (type == 2) /* song pointer -- 2 data bytes */
    {
      if (csysi_cnt < csysi_len)
	csysi_cnt++;
      else
	csysi_getbyte();
      if (csysi_cnt < csysi_len)
	csysi_cnt++;
      else
	csysi_getbyte();
      if (csysi_cnt == csysi_len)
	return csysi_midiparseover();
      else
	return CSYS_MIDIEVENTS;
    }

  if (type == 0) 
    {
      if (csysi_cnt < csysi_len)
	byte = csysi_data[csysi_cnt++];
      else
	byte = csysi_getbyte();
      while (byte < CSYS_MIDI_NOTEOFF)
	if (csysi_cnt < csysi_len)
	  byte = csysi_data[csysi_cnt++];
	else
	  byte = csysi_getbyte();
      if (byte != CSYSI_SYSEX_EOX) /* non-compliant MIDI */
	{
	  if ((byte&0xF0) != 0xF0)
	    {
	      csysi_cmd = byte&0xF0;
	      csysi_extchan = byte&0x0F;
	    }
	  switch (byte&0xF0) {
	  case CSYS_MIDI_NOTEOFF:
	  case CSYS_MIDI_NOTEON:
	  case CSYS_MIDI_PTOUCH:
	  case CSYS_MIDI_WHEEL:
	  case CSYS_MIDI_CC:
	    csysi_num = 2;
	    break;
	  case CSYS_MIDI_PROGRAM:
	  case CSYS_MIDI_CTOUCH:
	    csysi_num = 1;
	    break;
	  case 0xF0: 
	    if ((byte&0x0F)==2) /* song pointer -- 2 data bytes */
	      {
		if (csysi_cnt < csysi_len)
		  csysi_cnt++;
		else
		  csysi_getbyte();
		if (csysi_cnt < csysi_len)
		  csysi_cnt++;
		else
		  csysi_getbyte();
	      }
	    if ((byte&0x0F)==3) /* song select -- 1 data byte */
	      {
		if (csysi_cnt < csysi_len)
		  csysi_cnt++;
		else
		  csysi_getbyte();
	      }
	    break;
	  }
	}
    }

  /* outside of if {} to catch errant F7 bytes */

  if (csysi_cnt == csysi_len)
    return csysi_midiparseover();
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
  unsigned char overflow[2], oval;
  int len, tot, idx;


  if (csysi_data[csysi_cnt] > 127)    /* a command byte */
    {
      *cmd = 0xF0 & csysi_data[csysi_cnt];
      *extchan = 0x0F & csysi_data[csysi_cnt];
      if (*cmd != 0xF0)
	{
	  csysi_cmd = *cmd;
	  csysi_extchan = *extchan;
	}
      csysi_cnt++;
      switch (*cmd) {
      case CSYS_MIDI_NOTEOFF:
      case CSYS_MIDI_NOTEON:
      case CSYS_MIDI_PTOUCH:
      case CSYS_MIDI_WHEEL:
      case CSYS_MIDI_CC:
	csysi_num = 2;
	if (CSYSI_DELAY && ((csysi_cnt + 1) == csysi_len)) /* delay cmd */
	  {
	    csysi_ndata = csysi_data[csysi_cnt];
	    *cmd = CSYS_MIDI_NOOP;
	    return csysi_midiparseover();
	  }
	break;
      case CSYS_MIDI_PROGRAM:
      case CSYS_MIDI_CTOUCH:
	csysi_num = 1;
	break;
      case 0xF0: 
	*cmd = CSYS_MIDI_NOOP;
	return csysi_sysflush(*extchan);
	break;
      }
      if (CSYSI_DELAY && (csysi_cnt == csysi_len)) /* delay cmd */
	{
	  *cmd = CSYS_MIDI_NOOP;
	  return csysi_midiparseover();
	}
    }
  else  /* running status or a delayed MIDI command */
    {
      *cmd = csysi_cmd;
      *extchan = csysi_extchan;
      if (CSYSI_DELAY && (csysi_ndata != 0xFF)) /* finish delayed cmd */
	{
	  *ndata = csysi_ndata;
	  csysi_ndata = 0xFF;
	  csysi_newnote |= (((*cmd) == CSYS_MIDI_NOTEON) |
			    ((*cmd) == CSYS_MIDI_NOTEOFF));
	  *vdata = csysi_data[csysi_cnt++];
	  if (csysi_cnt == csysi_len)
	    return csysi_midiparseover();
	  else
	    return CSYS_MIDIEVENTS;
	}
      if (CSYSI_DELAY && (csysi_num == 2) && /* (further) delay cmd */
	  (csysi_cnt + 1 == csysi_len))
	{
	  csysi_ndata = csysi_data[csysi_cnt];
	  *cmd = CSYS_MIDI_NOOP;
	  return csysi_midiparseover();
	}
    }

  /* do complete commands and finish some types of delayed commands */

  if (csysi_cnt + csysi_num <= csysi_len)
    {
      csysi_newnote |= (((*cmd) == CSYS_MIDI_NOTEON) |
			((*cmd) == CSYS_MIDI_NOTEOFF));
      *ndata = csysi_data[csysi_cnt++];
      if (csysi_num == 2)
	*vdata = csysi_data[csysi_cnt++];
      if (csysi_cnt == csysi_len)
	return csysi_midiparseover();
      else
	return CSYS_MIDIEVENTS;
    }

  /* should never execute if CSYSI_DELAY is 1 */

  csysi_newnote |= (((*cmd) == CSYS_MIDI_NOTEON) |
		    ((*cmd) == CSYS_MIDI_NOTEOFF));

  tot = csysi_cnt + csysi_num - csysi_len;
  idx = 0;
  while (tot > 0)
    {
      overflow[idx++] = csysi_getbyte();
      tot--;
    }
  if (csysi_num == 1) 
    {
      *ndata = overflow[0];
      return csysi_midiparseover();
    }
  if (csysi_cnt + 1 == csysi_len)
    {
      *ndata = csysi_data[csysi_cnt++];
      *vdata = overflow[0];
    }
  else
    {
      *ndata = overflow[0];
      *vdata = overflow[1];
    }
  return csysi_midiparseover();
  
}


/****************************************************************/
/*                  closing routine for control                 */
/****************************************************************/

void csys_shutdown(void)
     
{
  /* disarm timer */

  if (sigprocmask(SIG_BLOCK, &csysi_overrun_mask, NULL) < 0)
    CSYSI_ERROR_TERMINATE("Couldn't mask MIDI overrun time");

  csysi_overrun_timer.it_value.tv_sec = 0;
  csysi_overrun_timer.it_value.tv_usec = 0;
  csysi_overrun_timer.it_interval.tv_sec = 0;
  csysi_overrun_timer.it_interval.tv_usec = 0;

  if (setitimer(ITIMER_REAL, &csysi_overrun_timer, NULL) < 0)
    CSYSI_ERROR_TERMINATE("Couldn't disarm ITIMER_REAL timer");

  close(csysi_midi);
}


#undef CSYSI_MIDIDEV
#undef CSYSI_BUFFSIZE
#undef CSYSI_SYSEX_EOX
#undef CSYSI_DELAY
#undef CSYSI_ALARMPERIOD
#undef CSYSI_ERROR_RETURN
#undef CSYSI_ERROR_TERMINATE



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



float yaxupaxo__sym_kline1(struct ninstr_types * nstate)
{
   float va_dur2;
   float va_x3;
   float va_dur3;
   float va_x4;
   float x2;
   float dur1;
   float x1;
   float ret;

   x1 =  0.0F ;
   dur1 =  0.05F ;
   x2 =  1.0F ;
   va_dur2 = NV(yaxupaxo__tvr0) ;
   va_x3 =  1.0F  ;
   va_dur3 =  0.0F  ;
   va_x4 =  0.5F  ;
   ret = 0.0F;
   if (NVI(yaxupaxo_kline1_first)>0)
   {
      NV(yaxupaxo_kline1_t) += KTIME;
      ret = (NV(yaxupaxo_kline1_outT) += NV(yaxupaxo_kline1_addK));
      if (NV(yaxupaxo_kline1_t) > NV(yaxupaxo_kline1_cdur))
       {
        while (NV(yaxupaxo_kline1_t) > NV(yaxupaxo_kline1_cdur))
         {
           NV(yaxupaxo_kline1_t) -= NV(yaxupaxo_kline1_cdur);
           switch(NVI(yaxupaxo_kline1_first))
      {
         case 1:
         NV(yaxupaxo_kline1_cdur) = va_dur2;
           NV(yaxupaxo_kline1_clp) = NV(yaxupaxo_kline1_crp);
           NV(yaxupaxo_kline1_crp) = va_x3;
           break;
         case 2:
         NV(yaxupaxo_kline1_cdur) = va_dur3;
           NV(yaxupaxo_kline1_clp) = NV(yaxupaxo_kline1_crp);
           NV(yaxupaxo_kline1_crp) = va_x4;
           break;
           default:
           NVI(yaxupaxo_kline1_first) = -100;
           NV(yaxupaxo_kline1_cdur) = NV(yaxupaxo_kline1_t) + 10000.0F;
           break;
           }
         NVI(yaxupaxo_kline1_first)++;
        }
        NV(yaxupaxo_kline1_mult)=(NV(yaxupaxo_kline1_crp) - NV(yaxupaxo_kline1_clp))/NV(yaxupaxo_kline1_cdur);
        ret = NV(yaxupaxo_kline1_outT) = NV(yaxupaxo_kline1_clp)+NV(yaxupaxo_kline1_mult)*NV(yaxupaxo_kline1_t);
        NV(yaxupaxo_kline1_addK) = NV(yaxupaxo_kline1_mult)*KTIME;
        if (NVI(yaxupaxo_kline1_first)<0)
          ret = 0.0F;
      }
   }
   if (NVI(yaxupaxo_kline1_first)==0)
     {
       NVI(yaxupaxo_kline1_first) = 1;
       ret = NV(yaxupaxo_kline1_outT) = NV(yaxupaxo_kline1_clp) = x1;
       NV(yaxupaxo_kline1_crp) = x2;
       NV(yaxupaxo_kline1_cdur) = dur1;
       if (dur1 > 0.0F)
         NV(yaxupaxo_kline1_addK) = KTIME*((x2 - x1)/dur1);
     }
   return((NV(yaxupaxo_kline1_return) = ret));

}



float yaxupaxo__sym_doscil2(struct ninstr_types * nstate)
{
   int t;
   float ret;
   int i;
   float index;

   t =  TBL_GBL_yaxurand ;

#undef AP1
#define AP1 gtables[t]

   if (NVI(yaxupaxo_doscil2_play) && ((NV(yaxupaxo_doscil2_p) += AP1.dmult) < 1.0F))
    {
      i = (int) (index = NV(yaxupaxo_doscil2_p)*AP1.lenf);
      ret = AP1.t[i];
      if (i < (AP1.len - 1))
        ret += (index - i)*(AP1.t[i+1] - ret);
      else
        ret -= (index - i)*AP1.t[AP1.len-1];
    }
   else
    {
      ret = 0.0F;
      if (NV(yaxupaxo_doscil2_p) == 0.0F)
      {
        NVI(yaxupaxo_doscil2_play) = 1;
        ret = AP1.t[0];
      }
      else
       NVI(yaxupaxo_doscil2_play) = 0;
    }
   return((NV(yaxupaxo_doscil2_return) = ret));

}



float string_kbd__sym_tablewrite4(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_string_vel ;

#undef AP1
#define AP1 gtables[t]

   index = NV(string_kbd__tvr0);
   val = NV(string_kbd_vval);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(string_kbd_tablewrite4_return) = ret));

}



float string_kbd__sym_tablewrite6(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_string_vel ;

#undef AP1
#define AP1 gtables[t]

   index = NV(string_kbd__tvr1);
   val = NV(string_kbd_vval);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(string_kbd_tablewrite6_return) = ret));

}



float string_audio__sym_string_resinit1(struct ninstr_types * nstate)
{


#undef OSP_string_audio_string_resinit1_a
#undef OSPI_string_audio_string_resinit1_a

#define OSP_string_audio_string_resinit1_a(x) nstate->v[string_audio_a + x].f
#define OSPI_string_audio_string_resinit1_a(x) nstate->v[string_audio_a + x].i


#undef OSP_string_audio_string_resinit1_b
#undef OSPI_string_audio_string_resinit1_b

#define OSP_string_audio_string_resinit1_b(x) nstate->v[string_audio_b + x].f
#define OSPI_string_audio_string_resinit1_b(x) nstate->v[string_audio_b + x].i


#undef OSP_string_audio_string_resinit1_g
#undef OSPI_string_audio_string_resinit1_g

#define OSP_string_audio_string_resinit1_g(x) nstate->v[string_audio_g + x].f
#define OSPI_string_audio_string_resinit1_g(x) nstate->v[string_audio_g + x].i


#undef OSP_string_audio_string_resinit1_notenum
#undef OSPI_string_audio_string_resinit1_notenum

#define OSP_string_audio_string_resinit1_notenum(x) nstate->v[string_audio_notenum].f
#define OSPI_string_audio_string_resinit1_notenum(x) nstate->v[string_audio_notenum].i

NV(string_audio_string_resinit1_norm) = ((gtables[ TBL_GBL_string_qscale ].t[((int)(OSP_string_audio_string_resinit1_notenum(0))
)]));
 NV(string_audio_string_resinit1_scale) = (globaltune*(float)(pow(2.0F, 8.333334e-02F*(-69.0F + OSP_string_audio_string_resinit1_notenum(0))))) *  4.545455e-03F ;
 NV(string_audio_string_resinit1_freq +  0 ) =  440.0F  * NV(string_audio_string_resinit1_scale);
 NV(string_audio_string_resinit1_q +  0 ) =  300.0F  * NV(string_audio_string_resinit1_norm);
 NV(string_audio_string_resinit1_freq +  1 ) =  880.0F  * NV(string_audio_string_resinit1_scale);
 NV(string_audio_string_resinit1_q +  1 ) =  300.0F  * NV(string_audio_string_resinit1_norm);
 NV(string_audio_string_resinit1_freq +  2 ) =  1320.0F  * NV(string_audio_string_resinit1_scale);
 NV(string_audio_string_resinit1_q +  2 ) =  300.0F  * NV(string_audio_string_resinit1_norm);
 NV(string_audio_string_resinit1_freq +  3 ) =  1760.0F  * NV(string_audio_string_resinit1_scale);
 NV(string_audio_string_resinit1_q +  3 ) =  300.0F  * NV(string_audio_string_resinit1_norm);
 NV(string_audio_string_resinit1_freq +  4 ) =  2200.0F  * NV(string_audio_string_resinit1_scale);
 NV(string_audio_string_resinit1_q +  4 ) =  300.0F  * NV(string_audio_string_resinit1_norm);
 NV(string_audio_string_resinit1_freq +  5 ) =  2640.0F  * NV(string_audio_string_resinit1_scale);
 NV(string_audio_string_resinit1_q +  5 ) =  300.0F  * NV(string_audio_string_resinit1_norm);
 NV(string_audio_string_resinit1_freq +  6 ) =  3080.0F  * NV(string_audio_string_resinit1_scale);
 NV(string_audio_string_resinit1_q +  6 ) =  320.0F  * NV(string_audio_string_resinit1_norm);
 NV(string_audio_string_resinit1_freq +  7 ) =  3520.0F  * NV(string_audio_string_resinit1_scale);
 NV(string_audio_string_resinit1_q +  7 ) =  300.0F  * NV(string_audio_string_resinit1_norm);
 NV(string_audio_string_resinit1_freq +  8 ) =  3960.0F  * NV(string_audio_string_resinit1_scale);
 NV(string_audio_string_resinit1_q +  8 ) =  190.0F  * NV(string_audio_string_resinit1_norm);
 NV(string_audio_string_resinit1_freq +  9 ) =  4400.0F  * NV(string_audio_string_resinit1_scale);
 NV(string_audio_string_resinit1_q +  9 ) =  300.0F  * NV(string_audio_string_resinit1_norm);
 NV(string_audio_string_resinit1_norm) = ((gtables[ TBL_GBL_string_gscale ].t[((int)(OSP_string_audio_string_resinit1_notenum(0))
)]));
 OSP_string_audio_string_resinit1_g(0 +  0 ) =  ( NV(string_audio_string_resinit1_freq +  0 ) <  2.205000e+04F )
 ? NV(string_audio_string_resinit1_norm) *  0.7F  :  0.0F ;
 OSP_string_audio_string_resinit1_g(0 +  1 ) =  ( NV(string_audio_string_resinit1_freq +  1 ) <  2.205000e+04F )
 ? NV(string_audio_string_resinit1_norm) *  0.8F  :  0.0F ;
 OSP_string_audio_string_resinit1_g(0 +  2 ) =  ( NV(string_audio_string_resinit1_freq +  2 ) <  2.205000e+04F )
 ? NV(string_audio_string_resinit1_norm) *  0.6F  :  0.0F ;
 OSP_string_audio_string_resinit1_g(0 +  3 ) =  ( NV(string_audio_string_resinit1_freq +  3 ) <  2.205000e+04F )
 ? NV(string_audio_string_resinit1_norm) *  0.7F  :  0.0F ;
 OSP_string_audio_string_resinit1_g(0 +  4 ) =  ( NV(string_audio_string_resinit1_freq +  4 ) <  2.205000e+04F )
 ? NV(string_audio_string_resinit1_norm) *  0.7F  :  0.0F ;
 OSP_string_audio_string_resinit1_g(0 +  5 ) =  ( NV(string_audio_string_resinit1_freq +  5 ) <  2.205000e+04F )
 ? NV(string_audio_string_resinit1_norm) *  0.8F  :  0.0F ;
 OSP_string_audio_string_resinit1_g(0 +  6 ) =  ( NV(string_audio_string_resinit1_freq +  6 ) <  2.205000e+04F )
 ? NV(string_audio_string_resinit1_norm) *  0.95F  :  0.0F ;
 OSP_string_audio_string_resinit1_g(0 +  7 ) =  ( NV(string_audio_string_resinit1_freq +  7 ) <  2.205000e+04F )
 ? NV(string_audio_string_resinit1_norm) *  0.76F  :  0.0F ;
 OSP_string_audio_string_resinit1_g(0 +  8 ) =  ( NV(string_audio_string_resinit1_freq +  8 ) <  2.205000e+04F )
 ? NV(string_audio_string_resinit1_norm) *  0.87F  :  0.0F ;
 OSP_string_audio_string_resinit1_g(0 +  9 ) =  ( NV(string_audio_string_resinit1_freq +  9 ) <  2.205000e+04F )
 ? NV(string_audio_string_resinit1_norm) *  0.76F  :  0.0F ;
 NV(string_audio_string_resinit1_j) =  0.0F ;
  while  ( NV(string_audio_string_resinit1_j) <  10.0F )
 { NV(string_audio_string_resinit1_r + ((int)(0.5F + (NV(string_audio_string_resinit1_j) ))))  = ((float)exp( - NV(string_audio_string_resinit1_freq + ((int)(0.5F + (NV(string_audio_string_resinit1_j) ))))  /  (  44100.0F  * NV(string_audio_string_resinit1_q + ((int)(0.5F + (NV(string_audio_string_resinit1_j) )))) )
)
);
 OSP_string_audio_string_resinit1_a(0 + ((int)(0.5F + (NV(string_audio_string_resinit1_j) ))))  =  2.0F  * NV(string_audio_string_resinit1_r + ((int)(0.5F + (NV(string_audio_string_resinit1_j) ))))  * ((float)cos( 6.283185e+00F  *  ( NV(string_audio_string_resinit1_freq + ((int)(0.5F + (NV(string_audio_string_resinit1_j) ))))  *  2.267574e-05F )
)
);
 OSP_string_audio_string_resinit1_b(0 + ((int)(0.5F + (NV(string_audio_string_resinit1_j) ))))  =  - NV(string_audio_string_resinit1_r + ((int)(0.5F + (NV(string_audio_string_resinit1_j) ))))  * NV(string_audio_string_resinit1_r + ((int)(0.5F + (NV(string_audio_string_resinit1_j) )))) ;
 NV(string_audio_string_resinit1_j) = NV(string_audio_string_resinit1_j) +  1.0F ;
 }
   NV(string_audio_string_resinit1_return + 0) =  0.0F ;
   return(NV(string_audio_string_resinit1_return));
}



float string_audio__sym_string_strikeinit2(struct ninstr_types * nstate)
{


#undef OSP_string_audio_string_strikeinit2_aa
#undef OSPI_string_audio_string_strikeinit2_aa

#define OSP_string_audio_string_strikeinit2_aa(x) nstate->v[string_audio_aa].f
#define OSPI_string_audio_string_strikeinit2_aa(x) nstate->v[string_audio_aa].i


#undef OSP_string_audio_string_strikeinit2_ab
#undef OSPI_string_audio_string_strikeinit2_ab

#define OSP_string_audio_string_strikeinit2_ab(x) nstate->v[string_audio_ab].f
#define OSPI_string_audio_string_strikeinit2_ab(x) nstate->v[string_audio_ab].i


#undef OSP_string_audio_string_strikeinit2_sg
#undef OSPI_string_audio_string_strikeinit2_sg

#define OSP_string_audio_string_strikeinit2_sg(x) nstate->v[string_audio_sg].f
#define OSPI_string_audio_string_strikeinit2_sg(x) nstate->v[string_audio_sg].i


#undef OSP_string_audio_string_strikeinit2_vw
#undef OSPI_string_audio_string_strikeinit2_vw

#define OSP_string_audio_string_strikeinit2_vw(x) nstate->v[string_audio_vw].f
#define OSPI_string_audio_string_strikeinit2_vw(x) nstate->v[string_audio_vw].i


#undef OSP_string_audio_string_strikeinit2_vwn
#undef OSPI_string_audio_string_strikeinit2_vwn

#define OSP_string_audio_string_strikeinit2_vwn(x) nstate->v[string_audio_vwn].f
#define OSPI_string_audio_string_strikeinit2_vwn(x) nstate->v[string_audio_vwn].i


#undef OSP_string_audio_string_strikeinit2_notenum
#undef OSPI_string_audio_string_strikeinit2_notenum

#define OSP_string_audio_string_strikeinit2_notenum(x) nstate->v[string_audio_notenum].f
#define OSPI_string_audio_string_strikeinit2_notenum(x) nstate->v[string_audio_notenum].i

OSP_string_audio_string_strikeinit2_aa(0) =  1.504101e+00F ;
 OSP_string_audio_string_strikeinit2_ab(0) =  (-5.655801e-01F) ;
 OSP_string_audio_string_strikeinit2_vw(0) =  7.874016e-03F ;
 OSP_string_audio_string_strikeinit2_sg(0) =  0.004F ;
 OSP_string_audio_string_strikeinit2_vwn(0) =  0.02F ;
    NV(string_audio_string_strikeinit2_return + 0) =  0.0F ;
   return(NV(string_audio_string_strikeinit2_return));
}



float string_audio__sym_rms3(struct ninstr_types * nstate)
{
   float x;
   float ret;
   int i;

   if (NVI(string_audio_rms3_kcyc))
   {
     ret = 0.0F;
     for (i=0; i < NT(TBL_string_audio_rms3_buffer).len;i++)
      ret +=  NT(TBL_string_audio_rms3_buffer).t[i]*NT(TBL_string_audio_rms3_buffer).t[i];
     ret = (float)sqrt(ret*NV(string_audio_rms3_scale));
   }
   else
   {
       i = NT(TBL_string_audio_rms3_buffer).len = ACYCLE;
    NT(TBL_string_audio_rms3_buffer).t=(float *)calloc(i,sizeof(float));
    NV(string_audio_rms3_scale) = 1.0F/NT(TBL_string_audio_rms3_buffer).len;
    NVI(string_audio_rms3_kcyc) = kcycleidx;
    ret = 0.0F;
   }
   return((NV(string_audio_rms3_return) = ret));

}



void string_audio__sym_rms3_spec(struct ninstr_types * nstate)
{
   float x;
   float ret;
   int i;

   x = NV(string_audio_out);
     i = NT(TBL_string_audio_rms3_buffer).tend++;
     if (NT(TBL_string_audio_rms3_buffer).tend == NT(TBL_string_audio_rms3_buffer).len)
       NT(TBL_string_audio_rms3_buffer).tend = 0;
     NT(TBL_string_audio_rms3_buffer).t[i] = x;

}



float string_audio__sym_string_strikeupdate4(struct ninstr_types * nstate)
{


#undef OSP_string_audio_string_strikeupdate4_ky
#undef OSPI_string_audio_string_strikeupdate4_ky

#define OSP_string_audio_string_strikeupdate4_ky(x) nstate->v[string_audio_ky + x].f
#define OSPI_string_audio_string_strikeupdate4_ky(x) nstate->v[string_audio_ky + x].i


#undef OSP_string_audio_string_strikeupdate4_nm
#undef OSPI_string_audio_string_strikeupdate4_nm

#define OSP_string_audio_string_strikeupdate4_nm(x) nstate->v[string_audio_nm].f
#define OSPI_string_audio_string_strikeupdate4_nm(x) nstate->v[string_audio_nm].i


#undef OSP_string_audio_string_strikeupdate4_silent
#undef OSPI_string_audio_string_strikeupdate4_silent

#define OSP_string_audio_string_strikeupdate4_silent(x) nstate->v[string_audio_silent].f
#define OSPI_string_audio_string_strikeupdate4_silent(x) nstate->v[string_audio_silent].i


#undef OSP_string_audio_string_strikeupdate4_notenum
#undef OSPI_string_audio_string_strikeupdate4_notenum

#define OSP_string_audio_string_strikeupdate4_notenum(x) nstate->v[string_audio_notenum].f
#define OSPI_string_audio_string_strikeupdate4_notenum(x) nstate->v[string_audio_notenum].i


#undef OSP_string_audio_string_strikeupdate4_vw
#undef OSPI_string_audio_string_strikeupdate4_vw

#define OSP_string_audio_string_strikeupdate4_vw(x) nstate->v[string_audio_vw].f
#define OSPI_string_audio_string_strikeupdate4_vw(x) nstate->v[string_audio_vw].i


#undef OSP_string_audio_string_strikeupdate4_vwn
#undef OSPI_string_audio_string_strikeupdate4_vwn

#define OSP_string_audio_string_strikeupdate4_vwn(x) nstate->v[string_audio_vwn].f
#define OSPI_string_audio_string_strikeupdate4_vwn(x) nstate->v[string_audio_vwn].i

NV(string_audio_string_strikeupdate4_count) = OSP_string_audio_string_strikeupdate4_silent(0) ?  ( NV(string_audio_string_strikeupdate4_count) +  1.0F )
 : string_audio_string_strikeupdate4__sym_max1(NSP);
  if  (  (  ( NV(string_audio_string_strikeupdate4_count) >  5.0F )
 &&  (  NS(iline->itime)  >  0.25F )
)
 || NV(string_audio_string_strikeupdate4_exit))
 {  if  (  ! NV(string_audio_string_strikeupdate4_exit))
 {   if (!NS(iline->released))
    NS(iline->turnoff) = 1;
NV(string_audio_string_strikeupdate4_exit) =  1.0F ;
 string_audio_string_strikeupdate4__sym_tablewrite3(NSP);
NG(GBL_string_poly) = NG(GBL_string_poly) -  1.0F ;
 }
OSP_string_audio_string_strikeupdate4_ky(0 +  0 ) =  0.0F ;
 }
 else  {  if  ( ((gtables[ TBL_GBL_string_vel ].t[((int)(OSP_string_audio_string_strikeupdate4_notenum(0))
)])) >  0.0F )
 { OSP_string_audio_string_strikeupdate4_ky(0 +  0 ) = OSP_string_audio_string_strikeupdate4_vw(0) * ((gtables[ TBL_GBL_string_vel ].t[((int)(OSP_string_audio_string_strikeupdate4_notenum(0))
)]));
 OSP_string_audio_string_strikeupdate4_nm(0) = OSP_string_audio_string_strikeupdate4_ky(0 +  0 ) * OSP_string_audio_string_strikeupdate4_vwn(0);
 string_audio_string_strikeupdate4__sym_tablewrite9(NSP);
}
 else  { OSP_string_audio_string_strikeupdate4_ky(0 +  0 ) =  0.0F ;
 }
}
   NV(string_audio_string_strikeupdate4_return + 0) =  0.0F ;
   return(NV(string_audio_string_strikeupdate4_return));
}



float string_audio_string_strikeupdate4__sym_max1(struct ninstr_types * nstate)
{
   float va_x2;
   float x1;
   float ret;

   x1 = NV(string_audio_string_strikeupdate4_count) -  1.0F ;
   va_x2 =  0.0F  ;
   x1 = (x1 > va_x2) ? x1 : va_x2;
   return(NV(string_audio_string_strikeupdate4_max1_return) = x1);

}



float string_audio_string_strikeupdate4__sym_tablewrite3(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_string_vel ;

#undef AP1
#define AP1 gtables[t]

   index = ((int)(OSP_string_audio_string_strikeupdate4_notenum(0))
);
   val =  (-1.0F) ;
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(string_audio_string_strikeupdate4_tablewrite3_return) = ret));

}



float string_audio_string_strikeupdate4__sym_tablewrite9(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_string_vel ;

#undef AP1
#define AP1 gtables[t]

   index = ((int)(OSP_string_audio_string_strikeupdate4_notenum(0))
);
   val =  0.0F ;
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(string_audio_string_strikeupdate4_tablewrite9_return) = ret));

}



float perc__sym_timeset1(struct ninstr_types * nstate)
{

perc_timeset1__sym_tablewrite2(NSP);
perc_timeset1__sym_tablewrite4(NSP);
perc_timeset1__sym_tablewrite6(NSP);
perc_timeset1__sym_tablewrite8(NSP);
perc_timeset1__sym_tablewrite10(NSP);
perc_timeset1__sym_tablewrite12(NSP);
perc_timeset1__sym_tablewrite14(NSP);
perc_timeset1__sym_tablewrite16(NSP);
perc_timeset1__sym_tablewrite18(NSP);
perc_timeset1__sym_tablewrite20(NSP);
perc_timeset1__sym_tablewrite22(NSP);
perc_timeset1__sym_tablewrite24(NSP);
perc_timeset1__sym_tablewrite26(NSP);
perc_timeset1__sym_tablewrite28(NSP);
perc_timeset1__sym_tablewrite30(NSP);
perc_timeset1__sym_tablewrite32(NSP);
perc_timeset1__sym_tablewrite34(NSP);
   NV(perc_timeset1_return + 0) =  0.0F ;
   return(NV(perc_timeset1_return));
}



float perc_timeset1__sym_tablewrite2(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  0.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_kick ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite2_return) = ret));

}



float perc_timeset1__sym_tablewrite4(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  1.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_snare ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite4_return) = ret));

}



float perc_timeset1__sym_tablewrite6(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  2.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_lotom ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite6_return) = ret));

}



float perc_timeset1__sym_tablewrite8(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  3.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_midtom ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite8_return) = ret));

}



float perc_timeset1__sym_tablewrite10(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  4.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_hitom ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite10_return) = ret));

}



float perc_timeset1__sym_tablewrite12(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  5.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_loconga ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite12_return) = ret));

}



float perc_timeset1__sym_tablewrite14(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  6.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_midconga ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite14_return) = ret));

}



float perc_timeset1__sym_tablewrite16(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  7.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_hiconga ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite16_return) = ret));

}



float perc_timeset1__sym_tablewrite18(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  8.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_rimshot ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite18_return) = ret));

}



float perc_timeset1__sym_tablewrite20(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  9.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_claves ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite20_return) = ret));

}



float perc_timeset1__sym_tablewrite22(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  10.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_clap ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite22_return) = ret));

}



float perc_timeset1__sym_tablewrite24(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  11.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_maracas ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite24_return) = ret));

}



float perc_timeset1__sym_tablewrite26(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  12.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_cowbell ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite26_return) = ret));

}



float perc_timeset1__sym_tablewrite28(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  13.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_ride ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite28_return) = ret));

}



float perc_timeset1__sym_tablewrite30(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  14.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_crash ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite30_return) = ret));

}



float perc_timeset1__sym_tablewrite32(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  15.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_hhopen ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite32_return) = ret));

}



float perc_timeset1__sym_tablewrite34(struct ninstr_types * nstate)
{
   float val;
   float index;
   int t;
   float ret;
   int i;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index =  16.0F ;
   val =  2.267574e-05F  * (gtables[ TBL_GBL_hhclosed ].lenf);
   i = ROUND(index);
   ret = AP1.t[i] = val;
   if (!i)
     AP1.t[AP1.len] = AP1.t[i];
   return((NV(perc_timeset1_tablewrite34_return) = ret));

}



float perc__sym_tableread2(struct ninstr_types * nstate)
{
   float index;
   int t;
   float ret;
     int i,len;

   t =  TBL_GBL_midimap ;

#undef AP1
#define AP1 gtables[t]

   index = NV(perc_pitch);
   i = (int)index;
   ret = AP1.t[i] + (index - i)*(AP1.t[i+1] - AP1.t[i]);
   return((NV(perc_tableread2_return) = ret));

}



float perc__sym_tableread3(struct ninstr_types * nstate)
{
   float index;
   int t;
   float ret;
     int i,len;

   t =  TBL_GBL_levels ;

#undef AP1
#define AP1 gtables[t]

   index = NV(perc_pmap);
   i = (int)index;
   ret = AP1.t[i] + (index - i)*(AP1.t[i+1] - AP1.t[i]);
   return((NV(perc_tableread3_return) = ret));

}



float perc__sym_tableread4(struct ninstr_types * nstate)
{
   float index;
   int t;
   float ret;
     int i,len;

   t =  TBL_GBL_azimuth ;

#undef AP1
#define AP1 gtables[t]

   index = NV(perc_pmap);
   i = (int)index;
   ret = AP1.t[i] + (index - i)*(AP1.t[i+1] - AP1.t[i]);
   return((NV(perc_tableread4_return) = ret));

}



float perc__sym_tableread5(struct ninstr_types * nstate)
{
   float index;
   int t;
   float ret;
     int i,len;

   t =  TBL_GBL_elevation ;

#undef AP1
#define AP1 gtables[t]

   index = NV(perc_pmap);
   i = (int)index;
   ret = AP1.t[i] + (index - i)*(AP1.t[i+1] - AP1.t[i]);
   return((NV(perc_tableread5_return) = ret));

}



float perc__sym_tableread6(struct ninstr_types * nstate)
{
   float index;
   int t;
   float ret;
     int i,len;

   t =  TBL_GBL_times ;

#undef AP1
#define AP1 gtables[t]

   index = NV(perc_kpmap);
   i = (int)index;
   ret = AP1.t[i] + (index - i)*(AP1.t[i+1] - AP1.t[i]);
   return((NV(perc_tableread6_return) = ret));

}



float perc__sym_doscil7(struct ninstr_types * nstate)
{
   int t;
   float ret;
   int i;
   float index;

   t = nstate->v[perc__tvr1].i;

#undef AP1
#define AP1 (*((struct tableinfo *)(t)))

   if (NVI(perc_doscil7_play) && ((NV(perc_doscil7_p) += AP1.dmult) < 1.0F))
    {
      i = (int) (index = NV(perc_doscil7_p)*AP1.lenf);
      ret = AP1.t[i];
      if (i < (AP1.len - 1))
        ret += (index - i)*(AP1.t[i+1] - ret);
      else
        ret -= (index - i)*AP1.t[AP1.len-1];
    }
   else
    {
      ret = 0.0F;
      if (NV(perc_doscil7_p) == 0.0F)
      {
        NVI(perc_doscil7_play) = 1;
        ret = AP1.t[0];
      }
      else
       NVI(perc_doscil7_play) = 0;
    }
   return((NV(perc_doscil7_return) = ret));

}



float perc__sym_spatialize8(struct ninstr_types * nstate)
{
   float distance;
   float elevation;
   float azimuth;
   float x;
   float ret;
     int i,len;
     float lpc,lpe,in,phi,theta,esum,ein;
     float pL,pR,aleft,aright,left,right,room;

   x = NV(perc_samp);
   azimuth = NV(perc_az);
   elevation = NV(perc_el);
   distance =  0.0F ;
   if (NVI(perc_spatialize8_kcyc) != kcycleidx)
   {
    if (!NVI(perc_spatialize8_kcyc))
    {
      i = NT(TBL_perc_spatialize8_d0).len = 1+(ROUND(6.803e-05F*ARATE));
      NT(TBL_perc_spatialize8_d0).t = (float *) calloc(i,sizeof(float));
      i = NT(TBL_perc_spatialize8_d1).len = 1+(ROUND(2.041e-04F*ARATE));
      NT(TBL_perc_spatialize8_d1).t = (float *) calloc(i,sizeof(float));
      i = NT(TBL_perc_spatialize8_d2).len = 1+(ROUND(2.721e-04F*ARATE));
      NT(TBL_perc_spatialize8_d2).t = (float *) calloc(i,sizeof(float));
      i = NT(TBL_perc_spatialize8_d3).len = 1+(ROUND(3.628e-04F*ARATE));
      NT(TBL_perc_spatialize8_d3).t = (float *) calloc(i,sizeof(float));
      i = NT(TBL_perc_spatialize8_d4).len = 1+(ROUND(4.082e-04F*ARATE));
      NT(TBL_perc_spatialize8_d4).t = (float *) calloc(i,sizeof(float));
      i = NT(TBL_perc_spatialize8_d5).len = 1+(ROUND(4.728e-04F*ARATE));
      NT(TBL_perc_spatialize8_d5).t = (float *) calloc(i,sizeof(float));
      i = NT(TBL_perc_spatialize8_d6).len = 1+(ROUND(4.728e-04F*ARATE));
      NT(TBL_perc_spatialize8_d6).t = (float *) calloc(i,sizeof(float));
      i = NT(TBL_perc_spatialize8_d7).len = 1+(ROUND(2.000e-02F*ARATE));
      NT(TBL_perc_spatialize8_d7).t = (float *) calloc(i,sizeof(float));
    }
    if ((!NVI(perc_spatialize8_kcyc))||(NV(perc_spatialize8_odis)!=distance))
     {
      NV(perc_spatialize8_odis) = distance;
      lpe = 2.199115e+5F*ATIME*(float)pow((distance>0.3F)?distance:0.3F,-0.666667F);
      lpc = 0.0031416F;
      if (lpe < 3.27249e-05F)
        lpc = 30557.8F;
      else
       if (lpe < 1.56765F)
         lpc = 1.0F/(float)tan(lpe);
      NV(perc_spatialize8_dis_b0)= 1.0F/(1.0F + 1.414214F*lpc + lpc*lpc);
      NV(perc_spatialize8_dis_b1)= 2.0F*NV(perc_spatialize8_dis_b0);
      NV(perc_spatialize8_dis_b2)= NV(perc_spatialize8_dis_b0);
      NV(perc_spatialize8_dis_a1)= 2.0F*NV(perc_spatialize8_dis_b0)*(1.0F - lpc*lpc);
      NV(perc_spatialize8_dis_a2)= NV(perc_spatialize8_dis_b0)*(1.0F - 1.414214F*lpc + lpc*lpc);
     }
    if ((!NVI(perc_spatialize8_kcyc))||(NV(perc_spatialize8_oaz)!=azimuth)
                      || (NV(perc_spatialize8_oel)!=elevation))
     {
      NV(perc_spatialize8_oel) = elevation;
      NV(perc_spatialize8_oaz) = azimuth;
      theta = (float)asin(cos(elevation)*sin(azimuth));
      phi = (float)atan2(sin(elevation),cos(elevation)*cos(azimuth));
      NV(perc_spatialize8_t0) = ARATE*(2.268e-05F*(float)cos(theta/2.0F)*(float)sin(1.000e+00F*(1.570796F-phi))+4.535e-05F);
      NVI(perc_spatialize8_i0) = (int)NV(perc_spatialize8_t0);
      NV(perc_spatialize8_t0) -= NVI(perc_spatialize8_i0);
      NV(perc_spatialize8_t1) = ARATE*(1.134e-04F*(float)cos(theta/2.0F)*(float)sin(5.000e-01F*(1.570796F-phi))+9.070e-05F);
      NVI(perc_spatialize8_i1) = (int)NV(perc_spatialize8_t1);
      NV(perc_spatialize8_t1) -= NVI(perc_spatialize8_i1);
      NV(perc_spatialize8_t2) = ARATE*(1.134e-04F*(float)cos(theta/2.0F)*(float)sin(5.000e-01F*(1.570796F-phi))+1.587e-04F);
      NVI(perc_spatialize8_i2) = (int)NV(perc_spatialize8_t2);
      NV(perc_spatialize8_t2) -= NVI(perc_spatialize8_i2);
      NV(perc_spatialize8_t3) = ARATE*(1.134e-04F*(float)cos(theta/2.0F)*(float)sin(5.000e-01F*(1.570796F-phi))+2.494e-04F);
      NVI(perc_spatialize8_i3) = (int)NV(perc_spatialize8_t3);
      NV(perc_spatialize8_t3) -= NVI(perc_spatialize8_i3);
      NV(perc_spatialize8_t4) = ARATE*(1.134e-04F*(float)cos(theta/2.0F)*(float)sin(5.000e-01F*(1.570796F-phi))+2.948e-04F);
      NVI(perc_spatialize8_i4) = (int)NV(perc_spatialize8_t4);
      NV(perc_spatialize8_t4) -= NVI(perc_spatialize8_i4);
      pL = 1.0F - (float)sin(theta);
      pR = 1.0F + (float)sin(theta);
      NV(perc_spatialize8_az_b0L) = 1.009e-05F*(8.820e+04F*pL + 1.087e+04F);
      NV(perc_spatialize8_az_b0R) = 1.009e-05F*(8.820e+04F*pR + 1.087e+04F);
      NV(perc_spatialize8_az_b1L) = 1.009e-05F*(- 8.820e+04F*pL + 1.087e+04F);
      NV(perc_spatialize8_az_b1R) = 1.009e-05F*(- 8.820e+04F*pR + 1.087e+04F);
      NV(perc_spatialize8_az_a1) = -7.805e-01F;
      if (theta > 0.0F)
      {
      NV(perc_spatialize8_t5) = ARATE*1.839e-04F*(1.0F + theta);
     NVI(perc_spatialize8_i5) = (int)NV(perc_spatialize8_t5);
     NV(perc_spatialize8_t5) -= NVI(perc_spatialize8_i5);
   NV(perc_spatialize8_t6) = ARATE*1.839e-04F*(1.0F - (float)sin(theta));
     NVI(perc_spatialize8_i6) = (int)NV(perc_spatialize8_t6);
     NV(perc_spatialize8_t6) -= NVI(perc_spatialize8_i6);
      }
      else
      {
      NV(perc_spatialize8_t5) = ARATE*1.839e-04F*(1.0F - theta);
     NVI(perc_spatialize8_i5) = (int)NV(perc_spatialize8_t5);
     NV(perc_spatialize8_t5) -= NVI(perc_spatialize8_i5);
   NV(perc_spatialize8_t6) = ARATE*1.839e-04F*(1.0F + (float)sin(theta));
     NVI(perc_spatialize8_i6) = (int)NV(perc_spatialize8_t6);
     NV(perc_spatialize8_t6) -= NVI(perc_spatialize8_i6);
     }
      }
      NVI(perc_spatialize8_kcyc) = kcycleidx;
    }
   in = NV(perc_spatialize8_dis_b0)*x + NV(perc_spatialize8_dis_d2);
   NV(perc_spatialize8_dis_d2)=NV(perc_spatialize8_dis_d1)-NV(perc_spatialize8_dis_a1)*in+NV(perc_spatialize8_dis_b1)*x;
   NV(perc_spatialize8_dis_d1) =            -NV(perc_spatialize8_dis_a2)*in+NV(perc_spatialize8_dis_b2)*x;
   i = NT(TBL_perc_spatialize8_d7).tend;
   room = 1.778e-01F*NT(TBL_perc_spatialize8_d7).t[i];
   NT(TBL_perc_spatialize8_d7).t[i] = in;

   if ((++NT(TBL_perc_spatialize8_d7).tend) == NT(TBL_perc_spatialize8_d7).len)
    NT(TBL_perc_spatialize8_d7).tend = 0;
   ein = in;

   len = NT(TBL_perc_spatialize8_d0).len;
   if ((i = NVI(perc_spatialize8_i0) + NT(TBL_perc_spatialize8_d0).tend) >=len) 
    i -= len;
   ret = NT(TBL_perc_spatialize8_d0).t[i];
    if ((i+1) < len)
      ret+= NV(perc_spatialize8_t0)*(NT(TBL_perc_spatialize8_d0).t[i+1] - ret);
    else
      ret+= NV(perc_spatialize8_t0)*(NT(TBL_perc_spatialize8_d0).t[0] - ret);
   ein += 5.000e-01F*ret;
   if ((i = --NT(TBL_perc_spatialize8_d0).tend) < 0)
    i = NT(TBL_perc_spatialize8_d0).tend = len - 1;
   NT(TBL_perc_spatialize8_d0).t[i] = in;

   len = NT(TBL_perc_spatialize8_d1).len;
   if ((i = NVI(perc_spatialize8_i1) + NT(TBL_perc_spatialize8_d1).tend) >=len) 
    i -= len;
   ret = NT(TBL_perc_spatialize8_d1).t[i];
    if ((i+1) < len)
      ret+= NV(perc_spatialize8_t1)*(NT(TBL_perc_spatialize8_d1).t[i+1] - ret);
    else
      ret+= NV(perc_spatialize8_t1)*(NT(TBL_perc_spatialize8_d1).t[0] - ret);
   ein += -1.000e+00F*ret;
   if ((i = --NT(TBL_perc_spatialize8_d1).tend) < 0)
    i = NT(TBL_perc_spatialize8_d1).tend = len - 1;
   NT(TBL_perc_spatialize8_d1).t[i] = in;

   len = NT(TBL_perc_spatialize8_d2).len;
   if ((i = NVI(perc_spatialize8_i2) + NT(TBL_perc_spatialize8_d2).tend) >=len) 
    i -= len;
   ret = NT(TBL_perc_spatialize8_d2).t[i];
    if ((i+1) < len)
      ret+= NV(perc_spatialize8_t2)*(NT(TBL_perc_spatialize8_d2).t[i+1] - ret);
    else
      ret+= NV(perc_spatialize8_t2)*(NT(TBL_perc_spatialize8_d2).t[0] - ret);
   ein += 5.000e-01F*ret;
   if ((i = --NT(TBL_perc_spatialize8_d2).tend) < 0)
    i = NT(TBL_perc_spatialize8_d2).tend = len - 1;
   NT(TBL_perc_spatialize8_d2).t[i] = in;

   len = NT(TBL_perc_spatialize8_d3).len;
   if ((i = NVI(perc_spatialize8_i3) + NT(TBL_perc_spatialize8_d3).tend) >=len) 
    i -= len;
   ret = NT(TBL_perc_spatialize8_d3).t[i];
    if ((i+1) < len)
      ret+= NV(perc_spatialize8_t3)*(NT(TBL_perc_spatialize8_d3).t[i+1] - ret);
    else
      ret+= NV(perc_spatialize8_t3)*(NT(TBL_perc_spatialize8_d3).t[0] - ret);
   ein += -2.500e-01F*ret;
   if ((i = --NT(TBL_perc_spatialize8_d3).tend) < 0)
    i = NT(TBL_perc_spatialize8_d3).tend = len - 1;
   NT(TBL_perc_spatialize8_d3).t[i] = in;

   len = NT(TBL_perc_spatialize8_d4).len;
   if ((i = NVI(perc_spatialize8_i4) + NT(TBL_perc_spatialize8_d4).tend) >=len) 
    i -= len;
   ret = NT(TBL_perc_spatialize8_d4).t[i];
    if ((i+1) < len)
      ret+= NV(perc_spatialize8_t4)*(NT(TBL_perc_spatialize8_d4).t[i+1] - ret);
    else
      ret+= NV(perc_spatialize8_t4)*(NT(TBL_perc_spatialize8_d4).t[0] - ret);
   ein += 2.500e-01F*ret;
   if ((i = --NT(TBL_perc_spatialize8_d4).tend) < 0)
    i = NT(TBL_perc_spatialize8_d4).tend = len - 1;
   NT(TBL_perc_spatialize8_d4).t[i] = in;

   aleft = NV(perc_spatialize8_az_b0L)*ein + NV(perc_spatialize8_az_d1L);
   NV(perc_spatialize8_az_d1L)= -NV(perc_spatialize8_az_a1)*aleft+NV(perc_spatialize8_az_b1L)*ein;
   aright = NV(perc_spatialize8_az_b0R)*ein + NV(perc_spatialize8_az_d1R);
   NV(perc_spatialize8_az_d1R)= -NV(perc_spatialize8_az_a1)*aright+NV(perc_spatialize8_az_b1R)*ein;
   len = NT(TBL_perc_spatialize8_d5).len;
   if ((i = NVI(perc_spatialize8_i5) + NT(TBL_perc_spatialize8_d5).tend) >=len) 
    i -= len;
   ret = NT(TBL_perc_spatialize8_d5).t[i];
    if ((i+1) < len)
      ret+= NV(perc_spatialize8_t5)*(NT(TBL_perc_spatialize8_d5).t[i+1] - ret);
    else
      ret+= NV(perc_spatialize8_t5)*(NT(TBL_perc_spatialize8_d5).t[0] - ret);
   left = ret;
   if ((i = --NT(TBL_perc_spatialize8_d5).tend) < 0)
    i = NT(TBL_perc_spatialize8_d5).tend = len - 1;
   NT(TBL_perc_spatialize8_d5).t[i] = aleft;

   len = NT(TBL_perc_spatialize8_d6).len;
   if ((i = NVI(perc_spatialize8_i6) + NT(TBL_perc_spatialize8_d6).tend) >=len) 
    i -= len;
   ret = NT(TBL_perc_spatialize8_d6).t[i];
    if ((i+1) < len)
      ret+= NV(perc_spatialize8_t6)*(NT(TBL_perc_spatialize8_d6).t[i+1] - ret);
    else
      ret+= NV(perc_spatialize8_t6)*(NT(TBL_perc_spatialize8_d6).t[0] - ret);
   right = ret;
   if ((i = --NT(TBL_perc_spatialize8_d6).tend) < 0)
    i = NT(TBL_perc_spatialize8_d6).tend = len - 1;
   NT(TBL_perc_spatialize8_d6).t[i] = aright;

    TB(0) += left + room;
    TB(1) += right + room;
   return((NV(perc_spatialize8_return) = ret));

}


#undef NS
#define NS(x)  ninstr[0].x
#undef NSP
#define NSP  /* void */
#undef NT
#define NT(x)  ninstr[0].t[x]
#undef NV
#define NV(x)  ninstr[0].v[x].f
#undef NVI
#define NVI(x)  ninstr[0].v[x].i
#undef NVU
#define NVU(x)  ninstr[0].v[x]
#undef NP
#define NP(x)  ninstr[0].v[x].f
#undef NPI
#define NPI(x)  ninstr[0].v[x].i



float rv1__sym_reverb1(void)
{
   float va_r0;
   float f0;
   float x;
   float ret;
   int i;
   float apout1,apout2,csum,fout,c,e;

   x =  TB(BUS_rvbus + 0);
   f0 =  6000.0F ;
   va_r0 =  2.25F  ;
   if (NT(TBL_rv1_reverb1_ap1).t == NULL)
   {
    i = NT(TBL_rv1_reverb1_ap1).len = ROUND(0.005F*ARATE);
    if (i<=0)
   epr(298,"perc.saol","reverb","Library error -- allpass 1");
    NT(TBL_rv1_reverb1_ap1).t = (float *) calloc(i,sizeof(float)); 
    i = NT(TBL_rv1_reverb1_ap2).len = ROUND(0.0017F*ARATE);
    if (i<=0)
   epr(298,"perc.saol","reverb","Library error -- allpass 2");
    NT(TBL_rv1_reverb1_ap2).t = (float *) calloc(i,sizeof(float)); 
   i = NT(TBL_rv1_reverb1_dline0_0).len = ROUND(0.030000*ARATE);
   NT(TBL_rv1_reverb1_dline0_0).t = (float *) calloc(i,sizeof(float));
   if (va_r0 <= 0)
   epr(298,"perc.saol","reverb","Negative reverberation time");
   NV(rv1_reverb1_g0_0) = (float)exp(-0.207233F/va_r0);

   i = NT(TBL_rv1_reverb1_dline0_1).len = ROUND(0.034300*ARATE);
   NT(TBL_rv1_reverb1_dline0_1).t = (float *) calloc(i,sizeof(float));
   if (va_r0 <= 0)
   epr(298,"perc.saol","reverb","Negative reverberation time");
   NV(rv1_reverb1_g0_1) = (float)exp(-0.236936F/va_r0);

   i = NT(TBL_rv1_reverb1_dline0_2).len = ROUND(0.039300*ARATE);
   NT(TBL_rv1_reverb1_dline0_2).t = (float *) calloc(i,sizeof(float));
   if (va_r0 <= 0)
   epr(298,"perc.saol","reverb","Negative reverberation time");
   NV(rv1_reverb1_g0_2) = (float)exp(-0.271475F/va_r0);

   i = NT(TBL_rv1_reverb1_dline0_3).len = ROUND(0.045000*ARATE);
   NT(TBL_rv1_reverb1_dline0_3).t = (float *) calloc(i,sizeof(float));
   if (va_r0 <= 0)
   epr(298,"perc.saol","reverb","Negative reverberation time");
   NV(rv1_reverb1_g0_3) = (float)exp(-0.310849F/va_r0);

   e = 3.141593F*f0*ATIME;
     c = 0.0031416F;
     if (e < 3.27249e-05F)
       c = 30557.8F;
     else
      if (e < 1.56765F)
        c = 1.0F/(float)tan((double)e);
     NV(rv1_reverb1_b0_0)= 1.0F/(1.0F + 1.414214F*c + c*c);
     NV(rv1_reverb1_b1_0)= 2.0F*NV(rv1_reverb1_b0_0);
     NV(rv1_reverb1_b2_0)= NV(rv1_reverb1_b0_0);
     NV(rv1_reverb1_a1_0)= 2.0F*NV(rv1_reverb1_b0_0)*(1.0F - c*c);
     NV(rv1_reverb1_a2_0)= NV(rv1_reverb1_b0_0)*(1.0F - 1.414214F*c + c*c);

   }
   i = NT(TBL_rv1_reverb1_ap1).tend;
   apout1 = NT(TBL_rv1_reverb1_ap1).t[i] - x*0.7F;
   NT(TBL_rv1_reverb1_ap1).t[i] = apout1*0.7F + x;
   NT(TBL_rv1_reverb1_ap1).tend= ++i;
   if (i==NT(TBL_rv1_reverb1_ap1).len)
    NT(TBL_rv1_reverb1_ap1).tend=0;
   i = NT(TBL_rv1_reverb1_ap2).tend;
   apout2 = NT(TBL_rv1_reverb1_ap2).t[i] - apout1*0.7F;
   NT(TBL_rv1_reverb1_ap2).t[i] = apout2*0.7F + apout1;
   NT(TBL_rv1_reverb1_ap2).tend= ++i;
   if (i==NT(TBL_rv1_reverb1_ap2).len)
    NT(TBL_rv1_reverb1_ap2).tend=0;

   ret = 0.0F;
   csum = 0.0F;
   i = NT(TBL_rv1_reverb1_dline0_0).tend;
   csum += NT(TBL_rv1_reverb1_dline0_0).t[i];
   NT(TBL_rv1_reverb1_dline0_0).t[i] = NT(TBL_rv1_reverb1_dline0_0).t[i]*NV(rv1_reverb1_g0_0)+apout2;
   NT(TBL_rv1_reverb1_dline0_0).tend = ++i;
   if (i==NT(TBL_rv1_reverb1_dline0_0).len)
   NT(TBL_rv1_reverb1_dline0_0).tend=0;

   i = NT(TBL_rv1_reverb1_dline0_1).tend;
   csum += NT(TBL_rv1_reverb1_dline0_1).t[i];
   NT(TBL_rv1_reverb1_dline0_1).t[i] = NT(TBL_rv1_reverb1_dline0_1).t[i]*NV(rv1_reverb1_g0_1)+apout2;
   NT(TBL_rv1_reverb1_dline0_1).tend = ++i;
   if (i==NT(TBL_rv1_reverb1_dline0_1).len)
   NT(TBL_rv1_reverb1_dline0_1).tend=0;

   i = NT(TBL_rv1_reverb1_dline0_2).tend;
   csum += NT(TBL_rv1_reverb1_dline0_2).t[i];
   NT(TBL_rv1_reverb1_dline0_2).t[i] = NT(TBL_rv1_reverb1_dline0_2).t[i]*NV(rv1_reverb1_g0_2)+apout2;
   NT(TBL_rv1_reverb1_dline0_2).tend = ++i;
   if (i==NT(TBL_rv1_reverb1_dline0_2).len)
   NT(TBL_rv1_reverb1_dline0_2).tend=0;

   i = NT(TBL_rv1_reverb1_dline0_3).tend;
   csum += NT(TBL_rv1_reverb1_dline0_3).t[i];
   NT(TBL_rv1_reverb1_dline0_3).t[i] = NT(TBL_rv1_reverb1_dline0_3).t[i]*NV(rv1_reverb1_g0_3)+apout2;
   NT(TBL_rv1_reverb1_dline0_3).tend = ++i;
   if (i==NT(TBL_rv1_reverb1_dline0_3).len)
   NT(TBL_rv1_reverb1_dline0_3).tend=0;

   csum *= 0.25F;
   fout = NV(rv1_reverb1_b0_0)*csum + NV(rv1_reverb1_d2_0);
   NV(rv1_reverb1_d2_0)=NV(rv1_reverb1_d1_0)-NV(rv1_reverb1_a1_0)*fout+NV(rv1_reverb1_b1_0)*csum;
   NV(rv1_reverb1_d1_0) = -NV(rv1_reverb1_a2_0)*fout+NV(rv1_reverb1_b2_0)*csum;
   ret += fout;
   return((NV(rv1_reverb1_return) = ret));

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

void yaxupaxo_ipass(struct ninstr_types * nstate)
{
   int i;

memset(&(NV(0)), 0, yaxupaxo_ENDVAR*sizeof(float));
memset(&(NT(0)), 0, yaxupaxo_ENDTBL*sizeof(struct tableinfo));
   NV(yaxupaxo_pitch) = 
   NS(iline->p[yaxupaxo_pitch]);
   NV(yaxupaxo_vel) = 
   NS(iline->p[yaxupaxo_vel]);
NV(yaxupaxo__tvr0) =  (  (  1.0F  * NV(yaxupaxo_vel))
 *  7.874016e-03F )
;
 
}

void string_kbd_ipass(struct ninstr_types * nstate)
{
   int i;

memset(&(NV(0)), 0, string_kbd_ENDVAR*sizeof(float));
memset(&(NT(0)), 0, string_kbd_ENDTBL*sizeof(struct tableinfo));
   NV(string_kbd_pitch) = 
   NS(iline->p[string_kbd_pitch]);
   NV(string_kbd_velocity) = 
   NS(iline->p[string_kbd_velocity]);
NV(string_kbd__tvr0) = ((int)(NV(string_kbd_pitch))
);
 NV(string_kbd__tvr1) = ((int)(NV(string_kbd_pitch))
);
 
}

void string_audio_ipass(struct ninstr_types * nstate)
{
   int i;

memset(&(NV(0)), 0, string_audio_ENDVAR*sizeof(float));
memset(&(NT(0)), 0, string_audio_ENDTBL*sizeof(struct tableinfo));
   NV(string_audio_notenum) = 
   NS(iline->p[string_audio_notenum]);
string_audio__sym_string_resinit1(NSP);
string_audio__sym_string_strikeinit2(NSP);

}

void perc_ipass(struct ninstr_types * nstate)
{
   int i;

memset(&(NV(0)), 0, perc_ENDVAR*sizeof(float));
memset(&(NT(0)), 0, perc_ENDTBL*sizeof(struct tableinfo));
   NV(perc_pitch) = 
   NS(iline->p[perc_pitch]);
   NV(perc_vel) = 
   NS(iline->p[perc_vel]);
NV(perc_pmap) = perc__sym_tableread2(NSP);
 NV(perc_scale) =  ( NV(perc_vel) *  7.874016e-03F )
 * perc__sym_tableread3(NSP);
 NV(perc_az) = perc__sym_tableread4(NSP);
 NV(perc_el) = perc__sym_tableread5(NSP);
 NV(perc__tvr0) = NV(perc_pmap) >=  0.0F ;
  if  ( NV(perc__tvr0))
 {    switch((int) (0.5F + NV(perc_pmap) )) { 
   case 0:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_kick ]));
     break;
   case 1:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_snare ]));
     break;
   case 2:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_lotom ]));
     break;
   case 3:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_midtom ]));
     break;
   case 4:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_hitom ]));
     break;
   case 5:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_loconga ]));
     break;
   case 6:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_midconga ]));
     break;
   case 7:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_hiconga ]));
     break;
   case 8:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_rimshot ]));
     break;
   case 9:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_claves ]));
     break;
   case 10:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_clap ]));
     break;
   case 11:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_maracas ]));
     break;
   case 12:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_cowbell ]));
     break;
   case 13:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_ride ]));
     break;
   case 14:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_crash ]));
     break;
   case 15:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_hhopen ]));
     break;
   case 16:
     NVI(perc__tvr1) = (int)(&(gtables[ TBL_GBL_hhclosed ]));
     break;
   default:
   epr(233,"perc.saol","tablemap","Tablemap index out of range");
   }

}

}


#undef NS
#define NS(x)  ninstr[0].x
#undef NSP
#define NSP  /* void */
#undef NT
#define NT(x)  ninstr[0].t[x]
#undef NV
#define NV(x)  ninstr[0].v[x].f
#undef NVI
#define NVI(x)  ninstr[0].v[x].i
#undef NVU
#define NVU(x)  ninstr[0].v[x]
#undef NP
#define NP(x)  ninstr[0].v[x].f
#undef NPI
#define NPI(x)  ninstr[0].v[x].i

void rv1_ipass(void)
{
   int i;

memset(&(NV(0)), 0, rv1_ENDVAR*sizeof(float));
memset(&(NT(0)), 0, rv1_ENDTBL*sizeof(struct tableinfo));

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

void yaxupaxo_kpass(struct ninstr_types * nstate)
{

   int i;

 if  (  ! NG(GBL_yaxuinit))
 { NG(GBL_yaxuenv) = yaxupaxo__sym_kline1(NSP);
 }

}

void string_kbd_kpass(struct ninstr_types * nstate)
{

   int i;

NV(string_kbd_vval) = NV(string_kbd_velocity);
 NV(string_kbd_kpitch) = NV(string_kbd_pitch);
  if  ( ((gtables[ TBL_GBL_string_vel ].t[((int)(NV(string_kbd_kpitch))
)])) ==  (-1.0F) )
 {  if  ( NG(GBL_string_poly) <  24.0F )
 { string_kbd__sym_tablewrite4(NSP);
   if ( d_string_audio___next == NULL)
     d_string_audio___next = d_string_audio___first = d_string_audio___last;
   else
   {
    d_string_audio___next = d_string_audio___first;
    while (d_string_audio___next < d_string_audio___end)
     {
       if ((d_string_audio___next->noteon == ALLDONE)||
           (d_string_audio___next->noteon == NOTUSEDYET))
        break;
       d_string_audio___next++;
     }
    if ((d_string_audio___next != d_string_audio___end) && (d_string_audio___next > d_string_audio___last))
      d_string_audio___last = d_string_audio___next;
   }
   if (d_string_audio___next != d_string_audio___end)
    {
   d_string_audio___next->starttime = scorebeats + ( 0.0F );
   d_string_audio___next->sdur =  (-1.0F) ;
   if (d_string_audio___next->sdur < 0.0F)
     d_string_audio___next->sdur = -1.0F;
   d_string_audio___next->p[string_audio_notenum] = NV(string_kbd_pitch);
   d_string_audio___next->abstime = 0.0F;
   d_string_audio___next->endtime = MAXENDTIME;
   d_string_audio___next->released = 0;
   d_string_audio___next->turnoff = 0;
   d_string_audio___next->launch = NOTLAUNCHED;
   d_string_audio___next->numchan = NS(iline->numchan);
   d_string_audio___next->notenum = 255 & (NS(iline->notenum));
   d_string_audio___next->preset  = NS(iline->preset);
   if (d_string_audio___next->starttime <= (scorebeats+scoremult))
   {
    d_string_audio___next->notestate = nextstate;
    d_string_audio___next->nstate = &ninstr[nextstate];
    ninstr[nextstate].iline = d_string_audio___next;
    d_string_audio___next->time = (kcycleidx-1)*KTIME;
    if (d_string_audio___next->sdur >= 0.0F)
      d_string_audio___next->endtime = scorebeats + d_string_audio___next->sdur;
    d_string_audio___next->kbirth = kcycleidx;
    d_string_audio___next->noteon = PLAYING;
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
    if (pass == KPASS)
     {
      pass = IPASS;
      string_audio_ipass(d_string_audio___next->nstate);
      pass = KPASS;
     }
    else
     {
     string_audio_ipass(d_string_audio___next->nstate);
     }
   }
   else
    d_string_audio___next->noteon = TOBEPLAYED;
  }
NG(GBL_string_poly) = NG(GBL_string_poly) +  1.0F ;
 }
}
 else  { string_kbd__sym_tablewrite6(NSP);
}
  if (!NS(iline->released))
    NS(iline->turnoff) = 1;

}

void string_audio_kpass(struct ninstr_types * nstate)
{

   int i;

   NS(iline->itime) = ((float)(kcycleidx - NS(iline->kbirth)))*KTIME;

NV(string_audio_silent) =  ( string_audio__sym_rms3(NSP) <  8e-4F )
;
 string_audio__sym_string_strikeupdate4(NSP);

}

void perc_kpass(struct ninstr_types * nstate)
{

   int i;

   NS(iline->itime) = ((float)(kcycleidx - NS(iline->kbirth)))*KTIME;

 if  (  ! NG(GBL_inittimes))
 { NG(GBL_inittimes) =  1.0F ;
 perc__sym_timeset1(NSP);
}
NV(perc_kpmap) = NV(perc_pmap);
 NV(perc_etime) = perc__sym_tableread6(NSP);
  if  (  (  NS(iline->itime)  > NV(perc_etime))
 &&  (  ! NV(perc_eflag))
)
 { NV(perc_eflag) =  1.0F ;
   if (!NS(iline->released))
    NS(iline->turnoff) = 1;
}
 if  (  (  ((float)NS(iline->released)) )
 &&  (  ! NV(perc_eflag))
)
 { NV(perc_eflag) =  1.0F ;
  if  ( NV(perc_etime) >  NS(iline->itime) )
 {   if ((NS(iline->sdur) < 0.0F)||
   (NS(iline->turnoff)&&NS(iline->released)))
  {
    NS(iline->endtime) = scorebase;
    NS(iline->endabs) = (kbase - 1)*KTIME;
    NS(iline->sdur) = 0.0F;
  }
  NS(iline->abstime) += 
NV(perc_etime) -  NS(iline->itime)   ;
   if (NS(iline->released))
    NS(iline->turnoff) = 0;
}
}

}


#undef NS
#define NS(x)  ninstr[0].x
#undef NSP
#define NSP  /* void */
#undef NT
#define NT(x)  ninstr[0].t[x]
#undef NV
#define NV(x)  ninstr[0].v[x].f
#undef NVI
#define NVI(x)  ninstr[0].v[x].i
#undef NVU
#define NVU(x)  ninstr[0].v[x]
#undef NP
#define NP(x)  ninstr[0].v[x].f
#undef NPI
#define NPI(x)  ninstr[0].v[x].i

void rv1_kpass(void)
{

   int i;


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

void yaxupaxo_apass(struct ninstr_types * nstate)
{

NV(yaxupaxo_y) = yaxupaxo__sym_doscil2(NSP);
 TB(BUS_output_bus + 0) += NV(yaxupaxo_y) * NG(GBL_yaxuenv);
TB(BUS_output_bus + 1) += NV(yaxupaxo_y) * NG(GBL_yaxuenv);
}

void string_audio_apass(struct ninstr_types * nstate)
{

string_audio__sym_rms3_spec(NSP);
;
NV(string_audio_ay) = NV(string_audio_aa) * NV(string_audio_ay1) + NV(string_audio_ab) * NV(string_audio_ay2) + NV(string_audio_ky +  0 );
 NV(string_audio_x) =  ( (2.0F*(RMULT*((float)rand()) - 0.5F)*(NV(string_audio_nm))) + NV(string_audio_sg))
 * NV(string_audio_ay);
 NV(string_audio_y + 0) = NV(string_audio_a + 0) * NV(string_audio_y1 + 0) + NV(string_audio_b + 0) * NV(string_audio_y2 + 0) + NV(string_audio_x);
 NV(string_audio_y + 1) = NV(string_audio_a + 1) * NV(string_audio_y1 + 1) + NV(string_audio_b + 1) * NV(string_audio_y2 + 1) + NV(string_audio_x);
 NV(string_audio_y + 2) = NV(string_audio_a + 2) * NV(string_audio_y1 + 2) + NV(string_audio_b + 2) * NV(string_audio_y2 + 2) + NV(string_audio_x);
 NV(string_audio_y + 3) = NV(string_audio_a + 3) * NV(string_audio_y1 + 3) + NV(string_audio_b + 3) * NV(string_audio_y2 + 3) + NV(string_audio_x);
 NV(string_audio_y + 4) = NV(string_audio_a + 4) * NV(string_audio_y1 + 4) + NV(string_audio_b + 4) * NV(string_audio_y2 + 4) + NV(string_audio_x);
 NV(string_audio_y + 5) = NV(string_audio_a + 5) * NV(string_audio_y1 + 5) + NV(string_audio_b + 5) * NV(string_audio_y2 + 5) + NV(string_audio_x);
 NV(string_audio_y + 6) = NV(string_audio_a + 6) * NV(string_audio_y1 + 6) + NV(string_audio_b + 6) * NV(string_audio_y2 + 6) + NV(string_audio_x);
 NV(string_audio_y + 7) = NV(string_audio_a + 7) * NV(string_audio_y1 + 7) + NV(string_audio_b + 7) * NV(string_audio_y2 + 7) + NV(string_audio_x);
 NV(string_audio_y + 8) = NV(string_audio_a + 8) * NV(string_audio_y1 + 8) + NV(string_audio_b + 8) * NV(string_audio_y2 + 8) + NV(string_audio_x);
 NV(string_audio_y + 9) = NV(string_audio_a + 9) * NV(string_audio_y1 + 9) + NV(string_audio_b + 9) * NV(string_audio_y2 + 9) + NV(string_audio_x);
 NV(string_audio_ay2) = NV(string_audio_ay1);
 NV(string_audio_ay1) =  ( ((float)fabs(NV(string_audio_ay))
) >  1e-30F )
 ? NV(string_audio_ay) :  0.0F ;
 NV(string_audio_ky +  0 ) =  0.0F ;
 NV(string_audio_y2 + 0) = NV(string_audio_y1 + 0);
 NV(string_audio_y2 + 1) = NV(string_audio_y1 + 1);
 NV(string_audio_y2 + 2) = NV(string_audio_y1 + 2);
 NV(string_audio_y2 + 3) = NV(string_audio_y1 + 3);
 NV(string_audio_y2 + 4) = NV(string_audio_y1 + 4);
 NV(string_audio_y2 + 5) = NV(string_audio_y1 + 5);
 NV(string_audio_y2 + 6) = NV(string_audio_y1 + 6);
 NV(string_audio_y2 + 7) = NV(string_audio_y1 + 7);
 NV(string_audio_y2 + 8) = NV(string_audio_y1 + 8);
 NV(string_audio_y2 + 9) = NV(string_audio_y1 + 9);
 NV(string_audio_y1 + 0) = NV(string_audio_y + 0);
 NV(string_audio_y1 + 1) = NV(string_audio_y + 1);
 NV(string_audio_y1 + 2) = NV(string_audio_y + 2);
 NV(string_audio_y1 + 3) = NV(string_audio_y + 3);
 NV(string_audio_y1 + 4) = NV(string_audio_y + 4);
 NV(string_audio_y1 + 5) = NV(string_audio_y + 5);
 NV(string_audio_y1 + 6) = NV(string_audio_y + 6);
 NV(string_audio_y1 + 7) = NV(string_audio_y + 7);
 NV(string_audio_y1 + 8) = NV(string_audio_y + 8);
 NV(string_audio_y1 + 9) = NV(string_audio_y + 9);
 NV(string_audio_sy + 0) = NV(string_audio_g + 0) * NV(string_audio_y + 0);
 NV(string_audio_sy + 1) = NV(string_audio_g + 1) * NV(string_audio_y + 1);
 NV(string_audio_sy + 2) = NV(string_audio_g + 2) * NV(string_audio_y + 2);
 NV(string_audio_sy + 3) = NV(string_audio_g + 3) * NV(string_audio_y + 3);
 NV(string_audio_sy + 4) = NV(string_audio_g + 4) * NV(string_audio_y + 4);
 NV(string_audio_sy + 5) = NV(string_audio_g + 5) * NV(string_audio_y + 5);
 NV(string_audio_sy + 6) = NV(string_audio_g + 6) * NV(string_audio_y + 6);
 NV(string_audio_sy + 7) = NV(string_audio_g + 7) * NV(string_audio_y + 7);
 NV(string_audio_sy + 8) = NV(string_audio_g + 8) * NV(string_audio_y + 8);
 NV(string_audio_sy + 9) = NV(string_audio_g + 9) * NV(string_audio_y + 9);
 NV(string_audio_out) =  ( NV(string_audio_sy +  0 ) + NV(string_audio_sy +  1 ) + NV(string_audio_sy +  2 ) + NV(string_audio_sy +  3 ) + NV(string_audio_sy +  4 ) + NV(string_audio_sy +  5 ) + NV(string_audio_sy +  6 ) + NV(string_audio_sy +  7 ) + NV(string_audio_sy +  8 ) + NV(string_audio_sy +  9 ))
;
 TB(BUS_output_bus + 0) += NV(string_audio_out);
TB(BUS_output_bus + 1) += NV(string_audio_out);
}

void perc_apass(struct ninstr_types * nstate)
{

 if  ( NV(perc__tvr0))
 { NV(perc_samp) = NV(perc_scale) * perc__sym_doscil7(NSP);
 perc__sym_spatialize8(NSP);
TB(BUS_rvbus + 0) += NV(perc_samp);
}
}


#undef NS
#define NS(x)  ninstr[0].x
#undef NSP
#define NSP  /* void */
#undef NT
#define NT(x)  ninstr[0].t[x]
#undef NV
#define NV(x)  ninstr[0].v[x].f
#undef NVI
#define NVI(x)  ninstr[0].v[x].i
#undef NVU
#define NVU(x)  ninstr[0].v[x]
#undef NP
#define NP(x)  ninstr[0].v[x].f
#undef NPI
#define NPI(x)  ninstr[0].v[x].i

void rv1_apass(void)
{

TB(BUS_output_bus + 0) +=  0.1F  * rv1__sym_reverb1(NSP);
TB(BUS_output_bus + 1) +=  0.1F  * NV(rv1_reverb1_return);
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

float table_global_times[18];

float table_global_midimap[129] = { 
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,0.0F,0.0F,8.0F,1.0F,10.0F,
1.0F,2.0F,16.0F,4.0F,16.0F,2.0F,15.0F,3.0F,
3.0F,14.0F,4.0F,13.0F,14.0F,13.0F,11.0F,14.0F,
12.0F,14.0F,-1.0F,13.0F,-1.0F,-1.0F,6.0F,6.0F,
5.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,11.0F,-1.0F,
-1.0F,-1.0F,-1.0F,9.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F};

float table_global_azimuth[18] = { 
0.0F,-4.000000e-01F,1.700000e+00F,3.000000e-01F,3.000000e-01F,-1.700000e+00F,-1.700000e+00F,-1.700000e+00F,
-7.000000e-01F,1.700000e+00F,-1.700000e+00F,1.700000e+00F,1.700000e+00F,0.0F,0.0F,-1.700000e+00F,
-1.700000e+00F,0.0F};

float table_global_elevation[18] = { 
-1.0F,0.0F,-4.000000e-01F,4.000000e-01F,8.000000e-01F,8.000000e-01F,8.000000e-01F,8.000000e-01F,
0.0F,8.000000e-01F,0.0F,8.000000e-01F,0.0F,1.0F,1.0F,5.000000e-01F,
3.000000e-01F,-1.0F};

float table_global_levels[18] = { 
1.0F,1.0F,5.000000e-01F,5.000000e-01F,5.000000e-01F,1.0F,1.0F,1.0F,
1.0F,1.0F,6.000000e-01F,4.000000e-01F,6.000000e-01F,7.000000e-01F,7.000000e-01F,3.000000e-01F,
1.0F,1.0F};

float table_global_string_qscale[129] = { 
1.967160e-02F,2.091992e-02F,2.224744e-02F,2.365922e-02F,2.516058e-02F,2.675721e-02F,2.845516e-02F,3.026086e-02F,
3.218115e-02F,3.422329e-02F,3.639502e-02F,3.870457e-02F,4.116067e-02F,4.377263e-02F,4.655034e-02F,4.950432e-02F,
5.264575e-02F,5.598653e-02F,5.953931e-02F,6.331754e-02F,6.733553e-02F,7.160849e-02F,7.615260e-02F,8.098507e-02F,
8.612420e-02F,9.158944e-02F,9.740150e-02F,1.035824e-01F,1.101555e-01F,1.171457e-01F,1.245795e-01F,1.324850e-01F,
1.408922e-01F,1.498329e-01F,1.593410e-01F,1.694524e-01F,1.802055e-01F,1.916409e-01F,2.038020e-01F,2.167348e-01F,
2.304883e-01F,2.451146e-01F,2.606690e-01F,2.772104e-01F,2.948016e-01F,3.135090e-01F,3.334036e-01F,3.545606e-01F,
3.770602e-01F,4.009876e-01F,4.264334e-01F,4.534939e-01F,4.822716e-01F,5.128754e-01F,5.454214e-01F,5.800326e-01F,
6.168401e-01F,6.559834e-01F,6.976106e-01F,7.418794e-01F,7.889574e-01F,8.390228e-01F,8.922653e-01F,9.488865e-01F,
1.009101e+00F,1.073136e+00F,1.141235e+00F,1.213655e+00F,1.290671e+00F,1.372573e+00F,1.459674e+00F,1.552302e+00F,
1.650807e+00F,1.755563e+00F,1.866968e+00F,1.985441e+00F,2.111433e+00F,2.245419e+00F,2.387908e+00F,2.539440e+00F,
2.700587e+00F,2.871960e+00F,3.054208e+00F,3.248021e+00F,3.454133e+00F,3.673325e+00F,3.906426e+00F,4.154319e+00F,
4.417943e+00F,4.698295e+00F,4.996438e+00F,5.313500e+00F,5.650683e+00F,6.009263e+00F,6.390597e+00F,6.796130e+00F,
7.227396e+00F,7.686031e+00F,8.173769e+00F,8.692458e+00F,9.244061e+00F,9.830668e+00F,1.045450e+01F,1.111792e+01F,
1.182344e+01F,1.257372e+01F,1.337162e+01F,1.422016e+01F,1.512254e+01F,1.608218e+01F,1.710272e+01F,1.818802e+01F,
1.934219e+01F,2.056960e+01F,2.187490e+01F,2.326303e+01F,2.473925e+01F,2.630915e+01F,2.797867e+01F,2.975413e+01F,
3.164226e+01F,3.365020e+01F,3.578557e+01F,3.805644e+01F,4.047141e+01F,4.303963e+01F,4.577083e+01F,4.867535e+01F,
1.967160e-02F};

float table_global_string_gscale[129] = { 
5.711536e-04F,6.147313e-04F,6.616340e-04F,7.121151e-04F,7.664479e-04F,8.249262e-04F,8.878662e-04F,9.556084e-04F,
1.028519e-03F,1.106993e-03F,1.191454e-03F,1.282359e-03F,1.380200e-03F,1.485506e-03F,1.598847e-03F,1.720836e-03F,
1.852132e-03F,1.993445e-03F,2.145540e-03F,2.309240e-03F,2.485430e-03F,2.675063e-03F,2.879165e-03F,3.098838e-03F,
3.335273e-03F,3.589747e-03F,3.863636e-03F,4.158423e-03F,4.475702e-03F,4.817188e-03F,5.184729e-03F,5.580312e-03F,
6.006077e-03F,6.464327e-03F,6.957541e-03F,7.488385e-03F,8.059733e-03F,8.674672e-03F,9.336530e-03F,1.004889e-02F,
1.081559e-02F,1.164080e-02F,1.252897e-02F,1.348490e-02F,1.451377e-02F,1.562113e-02F,1.681299e-02F,1.809579e-02F,
1.947645e-02F,2.096246e-02F,2.256185e-02F,2.428327e-02F,2.613603e-02F,2.813015e-02F,3.027641e-02F,3.258644e-02F,
3.507271e-02F,3.774868e-02F,4.062882e-02F,4.372871e-02F,4.706511e-02F,5.065608e-02F,5.452102e-02F,5.868085e-02F,
6.315807e-02F,6.797688e-02F,7.316337e-02F,7.874557e-02F,8.475368e-02F,9.122019e-02F,9.818009e-02F,1.056710e-01F,
1.137335e-01F,1.224111e-01F,1.317508e-01F,1.418031e-01F,1.526223e-01F,1.642670e-01F,1.768003e-01F,1.902897e-01F,
2.048084e-01F,2.204348e-01F,2.372535e-01F,2.553554e-01F,2.748385e-01F,2.958080e-01F,3.183775e-01F,3.426690e-01F,
3.688139e-01F,3.969536e-01F,4.272403e-01F,4.598377e-01F,4.949223e-01F,5.326838e-01F,5.733263e-01F,6.170698e-01F,
6.641509e-01F,7.148241e-01F,7.693636e-01F,8.280644e-01F,8.912438e-01F,9.592437e-01F,1.032432e+00F,1.111204e+00F,
1.195986e+00F,1.287238e+00F,1.385451e+00F,1.491158e+00F,1.604930e+00F,1.727382e+00F,1.859178e+00F,2.001029e+00F,
2.153703e+00F,2.318025e+00F,2.494885e+00F,2.685240e+00F,2.890117e+00F,3.110627e+00F,3.347961e+00F,3.603403e+00F,
3.878334e+00F,4.174242e+00F,4.492727e+00F,4.835512e+00F,5.204451e+00F,5.601539e+00F,6.028924e+00F,6.488917e+00F,
5.711536e-04F};

float table_global_string_vel[129] = { 
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,-1.0F,
-1.0F};



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

   int j;
   int yaxurand__sym_size;
   float yaxurand__sym_rounding;
   float yaxurand__sym_c1;
   float yaxurand__sym_x;
   float yaxurand__sym_y;
   int yaxurand__sym_dist;
   float yaxurand__sym_p1;
   float yaxurand__sym_p2;
   int kick__sym_size;
   float kick__sym_rounding;
   long kick__sym_skip;
   unsigned char kick__sym_c[2];
   short kick__sym_word;
   int snare__sym_size;
   float snare__sym_rounding;
   long snare__sym_skip;
   unsigned char snare__sym_c[2];
   short snare__sym_word;
   int lotom__sym_size;
   float lotom__sym_rounding;
   long lotom__sym_skip;
   unsigned char lotom__sym_c[2];
   short lotom__sym_word;
   int midtom__sym_size;
   float midtom__sym_rounding;
   long midtom__sym_skip;
   unsigned char midtom__sym_c[2];
   short midtom__sym_word;
   int hitom__sym_size;
   float hitom__sym_rounding;
   long hitom__sym_skip;
   unsigned char hitom__sym_c[2];
   short hitom__sym_word;
   int loconga__sym_size;
   float loconga__sym_rounding;
   long loconga__sym_skip;
   unsigned char loconga__sym_c[2];
   short loconga__sym_word;
   int midconga__sym_size;
   float midconga__sym_rounding;
   long midconga__sym_skip;
   unsigned char midconga__sym_c[2];
   short midconga__sym_word;
   int hiconga__sym_size;
   float hiconga__sym_rounding;
   long hiconga__sym_skip;
   unsigned char hiconga__sym_c[2];
   short hiconga__sym_word;
   int rimshot__sym_size;
   float rimshot__sym_rounding;
   long rimshot__sym_skip;
   unsigned char rimshot__sym_c[2];
   short rimshot__sym_word;
   int claves__sym_size;
   float claves__sym_rounding;
   long claves__sym_skip;
   unsigned char claves__sym_c[2];
   short claves__sym_word;
   int clap__sym_size;
   float clap__sym_rounding;
   long clap__sym_skip;
   unsigned char clap__sym_c[2];
   short clap__sym_word;
   int maracas__sym_size;
   float maracas__sym_rounding;
   long maracas__sym_skip;
   unsigned char maracas__sym_c[2];
   short maracas__sym_word;
   int cowbell__sym_size;
   float cowbell__sym_rounding;
   long cowbell__sym_skip;
   unsigned char cowbell__sym_c[2];
   short cowbell__sym_word;
   int ride__sym_size;
   float ride__sym_rounding;
   long ride__sym_skip;
   unsigned char ride__sym_c[2];
   short ride__sym_word;
   int crash__sym_size;
   float crash__sym_rounding;
   long crash__sym_skip;
   unsigned char crash__sym_c[2];
   short crash__sym_word;
   int hhopen__sym_size;
   float hhopen__sym_rounding;
   long hhopen__sym_skip;
   unsigned char hhopen__sym_c[2];
   short hhopen__sym_word;
   int hhclosed__sym_size;
   float hhclosed__sym_rounding;
   long hhclosed__sym_skip;
   unsigned char hhclosed__sym_c[2];
   short hhclosed__sym_word;

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
   yaxurand__sym_rounding =  16384.0F ;
   yaxurand__sym_size = ROUND(yaxurand__sym_rounding);
   yaxurand__sym_rounding =  1.0F ;
   yaxurand__sym_dist = ROUND(yaxurand__sym_rounding);
   yaxurand__sym_p1 =  (-1.000000e-01F) ;
   yaxurand__sym_p2 =  0.1F ;
    
   i = NT(TBL_GBL_yaxurand).len = yaxurand__sym_size;
   if (i < 1)
   epr(14,"perc.saol","table","Table length < 1");
   NT(TBL_GBL_yaxurand).stamp = scorebeats;
   NT(TBL_GBL_yaxurand).lenf = (float)(i);
   if (i>0)
   {
     NT(TBL_GBL_yaxurand).n = (i - 1.0F)/i;
     NT(TBL_GBL_yaxurand).diff = (i - 1.0F)/i;
     NT(TBL_GBL_yaxurand).tend = i - 1;
   }
   NT(TBL_GBL_yaxurand).t = (float *) malloc((i+1)*sizeof(float));
   NT(TBL_GBL_yaxurand).llmem = 1;
   i--;
    
     yaxurand__sym_c1 = yaxurand__sym_p2 - yaxurand__sym_p1;
   i = 0;
   while (i < yaxurand__sym_size)
    {
        NT(TBL_GBL_yaxurand).t[i] = yaxurand__sym_c1*RMULT*((float)rand()) + yaxurand__sym_p1;
     i++;
    }
     NT(TBL_GBL_yaxurand).t[NT(TBL_GBL_yaxurand).len] = NT(TBL_GBL_yaxurand).t[0];
   kick__sym_rounding =  (-1.0F) ;
   kick__sym_size = ROUND(kick__sym_rounding);
   NT(TBL_GBL_kick).stamp = scorebeats;
   NT(TBL_GBL_kick).sr = 44100;
   NT(TBL_GBL_kick).lenf = (float)(NT(TBL_GBL_kick).len = 11026);
   NT(TBL_GBL_kick).t = (float *)(calloc(11027, 4));
   NT(TBL_GBL_kick).dmult = 9.069472e-05F;
   NT(TBL_GBL_kick).lmult = 1.000000e+00F;
   NT(TBL_GBL_kick).tend = 11025;
   NT(TBL_GBL_kick).n = NT(TBL_GBL_kick).diff = 9.999093e-01F;
   sfile = fopen("bd.wav", "rb");
   if (sfile == NULL)
   epr(19,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(kick__sym_c,1,1,sfile))
   epr(19,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 11026; i++)
   {
   if (!rread(kick__sym_c,1,2,sfile))
   epr(19,"perc.saol","table","Corrupt samplefile");
        kick__sym_word = (short)kick__sym_c[0];
        kick__sym_word |= (short)kick__sym_c[1]<<8;
        NT(TBL_GBL_kick).t[i] = 3.051758e-5F*kick__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_kick).t[11026] = NT(TBL_GBL_kick).t[0];
   snare__sym_rounding =  (-1.0F) ;
   snare__sym_size = ROUND(snare__sym_rounding);
   NT(TBL_GBL_snare).stamp = scorebeats;
   NT(TBL_GBL_snare).sr = 44100;
   NT(TBL_GBL_snare).lenf = (float)(NT(TBL_GBL_snare).len = 22051);
   NT(TBL_GBL_snare).t = (float *)(calloc(22052, 4));
   NT(TBL_GBL_snare).dmult = 4.534942e-05F;
   NT(TBL_GBL_snare).lmult = 1.000000e+00F;
   NT(TBL_GBL_snare).tend = 22050;
   NT(TBL_GBL_snare).n = NT(TBL_GBL_snare).diff = 9.999547e-01F;
   sfile = fopen("sd.wav", "rb");
   if (sfile == NULL)
   epr(20,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(snare__sym_c,1,1,sfile))
   epr(20,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 22051; i++)
   {
   if (!rread(snare__sym_c,1,2,sfile))
   epr(20,"perc.saol","table","Corrupt samplefile");
        snare__sym_word = (short)snare__sym_c[0];
        snare__sym_word |= (short)snare__sym_c[1]<<8;
        NT(TBL_GBL_snare).t[i] = 3.051758e-5F*snare__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_snare).t[22051] = NT(TBL_GBL_snare).t[0];
   lotom__sym_rounding =  (-1.0F) ;
   lotom__sym_size = ROUND(lotom__sym_rounding);
   NT(TBL_GBL_lotom).stamp = scorebeats;
   NT(TBL_GBL_lotom).sr = 44100;
   NT(TBL_GBL_lotom).lenf = (float)(NT(TBL_GBL_lotom).len = 44101);
   NT(TBL_GBL_lotom).t = (float *)(calloc(44102, 4));
   NT(TBL_GBL_lotom).dmult = 2.267522e-05F;
   NT(TBL_GBL_lotom).lmult = 1.000000e+00F;
   NT(TBL_GBL_lotom).tend = 44100;
   NT(TBL_GBL_lotom).n = NT(TBL_GBL_lotom).diff = 9.999773e-01F;
   sfile = fopen("lt.wav", "rb");
   if (sfile == NULL)
   epr(21,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(lotom__sym_c,1,1,sfile))
   epr(21,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 44101; i++)
   {
   if (!rread(lotom__sym_c,1,2,sfile))
   epr(21,"perc.saol","table","Corrupt samplefile");
        lotom__sym_word = (short)lotom__sym_c[0];
        lotom__sym_word |= (short)lotom__sym_c[1]<<8;
        NT(TBL_GBL_lotom).t[i] = 3.051758e-5F*lotom__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_lotom).t[44101] = NT(TBL_GBL_lotom).t[0];
   midtom__sym_rounding =  (-1.0F) ;
   midtom__sym_size = ROUND(midtom__sym_rounding);
   NT(TBL_GBL_midtom).stamp = scorebeats;
   NT(TBL_GBL_midtom).sr = 44100;
   NT(TBL_GBL_midtom).lenf = (float)(NT(TBL_GBL_midtom).len = 44101);
   NT(TBL_GBL_midtom).t = (float *)(calloc(44102, 4));
   NT(TBL_GBL_midtom).dmult = 2.267522e-05F;
   NT(TBL_GBL_midtom).lmult = 1.000000e+00F;
   NT(TBL_GBL_midtom).tend = 44100;
   NT(TBL_GBL_midtom).n = NT(TBL_GBL_midtom).diff = 9.999773e-01F;
   sfile = fopen("mt.wav", "rb");
   if (sfile == NULL)
   epr(22,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(midtom__sym_c,1,1,sfile))
   epr(22,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 44101; i++)
   {
   if (!rread(midtom__sym_c,1,2,sfile))
   epr(22,"perc.saol","table","Corrupt samplefile");
        midtom__sym_word = (short)midtom__sym_c[0];
        midtom__sym_word |= (short)midtom__sym_c[1]<<8;
        NT(TBL_GBL_midtom).t[i] = 3.051758e-5F*midtom__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_midtom).t[44101] = NT(TBL_GBL_midtom).t[0];
   hitom__sym_rounding =  (-1.0F) ;
   hitom__sym_size = ROUND(hitom__sym_rounding);
   NT(TBL_GBL_hitom).stamp = scorebeats;
   NT(TBL_GBL_hitom).sr = 44100;
   NT(TBL_GBL_hitom).lenf = (float)(NT(TBL_GBL_hitom).len = 44101);
   NT(TBL_GBL_hitom).t = (float *)(calloc(44102, 4));
   NT(TBL_GBL_hitom).dmult = 2.267522e-05F;
   NT(TBL_GBL_hitom).lmult = 1.000000e+00F;
   NT(TBL_GBL_hitom).tend = 44100;
   NT(TBL_GBL_hitom).n = NT(TBL_GBL_hitom).diff = 9.999773e-01F;
   sfile = fopen("ht.wav", "rb");
   if (sfile == NULL)
   epr(23,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(hitom__sym_c,1,1,sfile))
   epr(23,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 44101; i++)
   {
   if (!rread(hitom__sym_c,1,2,sfile))
   epr(23,"perc.saol","table","Corrupt samplefile");
        hitom__sym_word = (short)hitom__sym_c[0];
        hitom__sym_word |= (short)hitom__sym_c[1]<<8;
        NT(TBL_GBL_hitom).t[i] = 3.051758e-5F*hitom__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_hitom).t[44101] = NT(TBL_GBL_hitom).t[0];
   loconga__sym_rounding =  (-1.0F) ;
   loconga__sym_size = ROUND(loconga__sym_rounding);
   NT(TBL_GBL_loconga).stamp = scorebeats;
   NT(TBL_GBL_loconga).sr = 44100;
   NT(TBL_GBL_loconga).lenf = (float)(NT(TBL_GBL_loconga).len = 22051);
   NT(TBL_GBL_loconga).t = (float *)(calloc(22052, 4));
   NT(TBL_GBL_loconga).dmult = 4.534942e-05F;
   NT(TBL_GBL_loconga).lmult = 1.000000e+00F;
   NT(TBL_GBL_loconga).tend = 22050;
   NT(TBL_GBL_loconga).n = NT(TBL_GBL_loconga).diff = 9.999547e-01F;
   sfile = fopen("lc.wav", "rb");
   if (sfile == NULL)
   epr(24,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(loconga__sym_c,1,1,sfile))
   epr(24,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 22051; i++)
   {
   if (!rread(loconga__sym_c,1,2,sfile))
   epr(24,"perc.saol","table","Corrupt samplefile");
        loconga__sym_word = (short)loconga__sym_c[0];
        loconga__sym_word |= (short)loconga__sym_c[1]<<8;
        NT(TBL_GBL_loconga).t[i] = 3.051758e-5F*loconga__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_loconga).t[22051] = NT(TBL_GBL_loconga).t[0];
   midconga__sym_rounding =  (-1.0F) ;
   midconga__sym_size = ROUND(midconga__sym_rounding);
   NT(TBL_GBL_midconga).stamp = scorebeats;
   NT(TBL_GBL_midconga).sr = 44100;
   NT(TBL_GBL_midconga).lenf = (float)(NT(TBL_GBL_midconga).len = 22051);
   NT(TBL_GBL_midconga).t = (float *)(calloc(22052, 4));
   NT(TBL_GBL_midconga).dmult = 4.534942e-05F;
   NT(TBL_GBL_midconga).lmult = 1.000000e+00F;
   NT(TBL_GBL_midconga).tend = 22050;
   NT(TBL_GBL_midconga).n = NT(TBL_GBL_midconga).diff = 9.999547e-01F;
   sfile = fopen("mc.wav", "rb");
   if (sfile == NULL)
   epr(25,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(midconga__sym_c,1,1,sfile))
   epr(25,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 22051; i++)
   {
   if (!rread(midconga__sym_c,1,2,sfile))
   epr(25,"perc.saol","table","Corrupt samplefile");
        midconga__sym_word = (short)midconga__sym_c[0];
        midconga__sym_word |= (short)midconga__sym_c[1]<<8;
        NT(TBL_GBL_midconga).t[i] = 3.051758e-5F*midconga__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_midconga).t[22051] = NT(TBL_GBL_midconga).t[0];
   hiconga__sym_rounding =  (-1.0F) ;
   hiconga__sym_size = ROUND(hiconga__sym_rounding);
   NT(TBL_GBL_hiconga).stamp = scorebeats;
   NT(TBL_GBL_hiconga).sr = 44100;
   NT(TBL_GBL_hiconga).lenf = (float)(NT(TBL_GBL_hiconga).len = 22051);
   NT(TBL_GBL_hiconga).t = (float *)(calloc(22052, 4));
   NT(TBL_GBL_hiconga).dmult = 4.534942e-05F;
   NT(TBL_GBL_hiconga).lmult = 1.000000e+00F;
   NT(TBL_GBL_hiconga).tend = 22050;
   NT(TBL_GBL_hiconga).n = NT(TBL_GBL_hiconga).diff = 9.999547e-01F;
   sfile = fopen("hc.wav", "rb");
   if (sfile == NULL)
   epr(26,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(hiconga__sym_c,1,1,sfile))
   epr(26,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 22051; i++)
   {
   if (!rread(hiconga__sym_c,1,2,sfile))
   epr(26,"perc.saol","table","Corrupt samplefile");
        hiconga__sym_word = (short)hiconga__sym_c[0];
        hiconga__sym_word |= (short)hiconga__sym_c[1]<<8;
        NT(TBL_GBL_hiconga).t[i] = 3.051758e-5F*hiconga__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_hiconga).t[22051] = NT(TBL_GBL_hiconga).t[0];
   rimshot__sym_rounding =  (-1.0F) ;
   rimshot__sym_size = ROUND(rimshot__sym_rounding);
   NT(TBL_GBL_rimshot).stamp = scorebeats;
   NT(TBL_GBL_rimshot).sr = 44100;
   NT(TBL_GBL_rimshot).lenf = (float)(NT(TBL_GBL_rimshot).len = 11026);
   NT(TBL_GBL_rimshot).t = (float *)(calloc(11027, 4));
   NT(TBL_GBL_rimshot).dmult = 9.069472e-05F;
   NT(TBL_GBL_rimshot).lmult = 1.000000e+00F;
   NT(TBL_GBL_rimshot).tend = 11025;
   NT(TBL_GBL_rimshot).n = NT(TBL_GBL_rimshot).diff = 9.999093e-01F;
   sfile = fopen("rs.wav", "rb");
   if (sfile == NULL)
   epr(27,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(rimshot__sym_c,1,1,sfile))
   epr(27,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 11026; i++)
   {
   if (!rread(rimshot__sym_c,1,2,sfile))
   epr(27,"perc.saol","table","Corrupt samplefile");
        rimshot__sym_word = (short)rimshot__sym_c[0];
        rimshot__sym_word |= (short)rimshot__sym_c[1]<<8;
        NT(TBL_GBL_rimshot).t[i] = 3.051758e-5F*rimshot__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_rimshot).t[11026] = NT(TBL_GBL_rimshot).t[0];
   claves__sym_rounding =  (-1.0F) ;
   claves__sym_size = ROUND(claves__sym_rounding);
   NT(TBL_GBL_claves).stamp = scorebeats;
   NT(TBL_GBL_claves).sr = 44100;
   NT(TBL_GBL_claves).lenf = (float)(NT(TBL_GBL_claves).len = 11025);
   NT(TBL_GBL_claves).t = (float *)(calloc(11026, 4));
   NT(TBL_GBL_claves).dmult = 9.070295e-05F;
   NT(TBL_GBL_claves).lmult = 1.000000e+00F;
   NT(TBL_GBL_claves).tend = 11024;
   NT(TBL_GBL_claves).n = NT(TBL_GBL_claves).diff = 9.999093e-01F;
   sfile = fopen("cl.wav", "rb");
   if (sfile == NULL)
   epr(28,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(claves__sym_c,1,1,sfile))
   epr(28,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 11025; i++)
   {
   if (!rread(claves__sym_c,1,2,sfile))
   epr(28,"perc.saol","table","Corrupt samplefile");
        claves__sym_word = (short)claves__sym_c[0];
        claves__sym_word |= (short)claves__sym_c[1]<<8;
        NT(TBL_GBL_claves).t[i] = 3.051758e-5F*claves__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_claves).t[11025] = NT(TBL_GBL_claves).t[0];
   clap__sym_rounding =  (-1.0F) ;
   clap__sym_size = ROUND(clap__sym_rounding);
   NT(TBL_GBL_clap).stamp = scorebeats;
   NT(TBL_GBL_clap).sr = 44100;
   NT(TBL_GBL_clap).lenf = (float)(NT(TBL_GBL_clap).len = 88201);
   NT(TBL_GBL_clap).t = (float *)(calloc(88202, 4));
   NT(TBL_GBL_clap).dmult = 1.133774e-05F;
   NT(TBL_GBL_clap).lmult = 1.000000e+00F;
   NT(TBL_GBL_clap).tend = 88200;
   NT(TBL_GBL_clap).n = NT(TBL_GBL_clap).diff = 9.999887e-01F;
   sfile = fopen("cp.wav", "rb");
   if (sfile == NULL)
   epr(29,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(clap__sym_c,1,1,sfile))
   epr(29,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 88201; i++)
   {
   if (!rread(clap__sym_c,1,2,sfile))
   epr(29,"perc.saol","table","Corrupt samplefile");
        clap__sym_word = (short)clap__sym_c[0];
        clap__sym_word |= (short)clap__sym_c[1]<<8;
        NT(TBL_GBL_clap).t[i] = 3.051758e-5F*clap__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_clap).t[88201] = NT(TBL_GBL_clap).t[0];
   maracas__sym_rounding =  (-1.0F) ;
   maracas__sym_size = ROUND(maracas__sym_rounding);
   NT(TBL_GBL_maracas).stamp = scorebeats;
   NT(TBL_GBL_maracas).sr = 44100;
   NT(TBL_GBL_maracas).lenf = (float)(NT(TBL_GBL_maracas).len = 11025);
   NT(TBL_GBL_maracas).t = (float *)(calloc(11026, 4));
   NT(TBL_GBL_maracas).dmult = 9.070295e-05F;
   NT(TBL_GBL_maracas).lmult = 1.000000e+00F;
   NT(TBL_GBL_maracas).tend = 11024;
   NT(TBL_GBL_maracas).n = NT(TBL_GBL_maracas).diff = 9.999093e-01F;
   sfile = fopen("ma.wav", "rb");
   if (sfile == NULL)
   epr(30,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(maracas__sym_c,1,1,sfile))
   epr(30,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 11025; i++)
   {
   if (!rread(maracas__sym_c,1,2,sfile))
   epr(30,"perc.saol","table","Corrupt samplefile");
        maracas__sym_word = (short)maracas__sym_c[0];
        maracas__sym_word |= (short)maracas__sym_c[1]<<8;
        NT(TBL_GBL_maracas).t[i] = 3.051758e-5F*maracas__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_maracas).t[11025] = NT(TBL_GBL_maracas).t[0];
   cowbell__sym_rounding =  (-1.0F) ;
   cowbell__sym_size = ROUND(cowbell__sym_rounding);
   NT(TBL_GBL_cowbell).stamp = scorebeats;
   NT(TBL_GBL_cowbell).sr = 44100;
   NT(TBL_GBL_cowbell).lenf = (float)(NT(TBL_GBL_cowbell).len = 66151);
   NT(TBL_GBL_cowbell).t = (float *)(calloc(66152, 4));
   NT(TBL_GBL_cowbell).dmult = 1.511693e-05F;
   NT(TBL_GBL_cowbell).lmult = 1.000000e+00F;
   NT(TBL_GBL_cowbell).tend = 66150;
   NT(TBL_GBL_cowbell).n = NT(TBL_GBL_cowbell).diff = 9.999849e-01F;
   sfile = fopen("cb.wav", "rb");
   if (sfile == NULL)
   epr(31,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(cowbell__sym_c,1,1,sfile))
   epr(31,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 66151; i++)
   {
   if (!rread(cowbell__sym_c,1,2,sfile))
   epr(31,"perc.saol","table","Corrupt samplefile");
        cowbell__sym_word = (short)cowbell__sym_c[0];
        cowbell__sym_word |= (short)cowbell__sym_c[1]<<8;
        NT(TBL_GBL_cowbell).t[i] = 3.051758e-5F*cowbell__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_cowbell).t[66151] = NT(TBL_GBL_cowbell).t[0];
   ride__sym_rounding =  (-1.0F) ;
   ride__sym_size = ROUND(ride__sym_rounding);
   NT(TBL_GBL_ride).stamp = scorebeats;
   NT(TBL_GBL_ride).sr = 44100;
   NT(TBL_GBL_ride).lenf = (float)(NT(TBL_GBL_ride).len = 66151);
   NT(TBL_GBL_ride).t = (float *)(calloc(66152, 4));
   NT(TBL_GBL_ride).dmult = 1.511693e-05F;
   NT(TBL_GBL_ride).lmult = 1.000000e+00F;
   NT(TBL_GBL_ride).tend = 66150;
   NT(TBL_GBL_ride).n = NT(TBL_GBL_ride).diff = 9.999849e-01F;
   sfile = fopen("cr.wav", "rb");
   if (sfile == NULL)
   epr(32,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(ride__sym_c,1,1,sfile))
   epr(32,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 66151; i++)
   {
   if (!rread(ride__sym_c,1,2,sfile))
   epr(32,"perc.saol","table","Corrupt samplefile");
        ride__sym_word = (short)ride__sym_c[0];
        ride__sym_word |= (short)ride__sym_c[1]<<8;
        NT(TBL_GBL_ride).t[i] = 3.051758e-5F*ride__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_ride).t[66151] = NT(TBL_GBL_ride).t[0];
   crash__sym_rounding =  (-1.0F) ;
   crash__sym_size = ROUND(crash__sym_rounding);
   NT(TBL_GBL_crash).stamp = scorebeats;
   NT(TBL_GBL_crash).sr = 44100;
   NT(TBL_GBL_crash).lenf = (float)(NT(TBL_GBL_crash).len = 154351);
   NT(TBL_GBL_crash).t = (float *)(calloc(154352, 4));
   NT(TBL_GBL_crash).dmult = 6.478740e-06F;
   NT(TBL_GBL_crash).lmult = 1.000000e+00F;
   NT(TBL_GBL_crash).tend = 154350;
   NT(TBL_GBL_crash).n = NT(TBL_GBL_crash).diff = 9.999935e-01F;
   sfile = fopen("cy.wav", "rb");
   if (sfile == NULL)
   epr(33,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(crash__sym_c,1,1,sfile))
   epr(33,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 154351; i++)
   {
   if (!rread(crash__sym_c,1,2,sfile))
   epr(33,"perc.saol","table","Corrupt samplefile");
        crash__sym_word = (short)crash__sym_c[0];
        crash__sym_word |= (short)crash__sym_c[1]<<8;
        NT(TBL_GBL_crash).t[i] = 3.051758e-5F*crash__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_crash).t[154351] = NT(TBL_GBL_crash).t[0];
   hhopen__sym_rounding =  (-1.0F) ;
   hhopen__sym_size = ROUND(hhopen__sym_rounding);
   NT(TBL_GBL_hhopen).stamp = scorebeats;
   NT(TBL_GBL_hhopen).sr = 44100;
   NT(TBL_GBL_hhopen).lenf = (float)(NT(TBL_GBL_hhopen).len = 22051);
   NT(TBL_GBL_hhopen).t = (float *)(calloc(22052, 4));
   NT(TBL_GBL_hhopen).dmult = 4.534942e-05F;
   NT(TBL_GBL_hhopen).lmult = 1.000000e+00F;
   NT(TBL_GBL_hhopen).tend = 22050;
   NT(TBL_GBL_hhopen).n = NT(TBL_GBL_hhopen).diff = 9.999547e-01F;
   sfile = fopen("oh.wav", "rb");
   if (sfile == NULL)
   epr(34,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(hhopen__sym_c,1,1,sfile))
   epr(34,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 22051; i++)
   {
   if (!rread(hhopen__sym_c,1,2,sfile))
   epr(34,"perc.saol","table","Corrupt samplefile");
        hhopen__sym_word = (short)hhopen__sym_c[0];
        hhopen__sym_word |= (short)hhopen__sym_c[1]<<8;
        NT(TBL_GBL_hhopen).t[i] = 3.051758e-5F*hhopen__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_hhopen).t[22051] = NT(TBL_GBL_hhopen).t[0];
   hhclosed__sym_rounding =  (-1.0F) ;
   hhclosed__sym_size = ROUND(hhclosed__sym_rounding);
   NT(TBL_GBL_hhclosed).stamp = scorebeats;
   NT(TBL_GBL_hhclosed).sr = 44100;
   NT(TBL_GBL_hhclosed).lenf = (float)(NT(TBL_GBL_hhclosed).len = 11025);
   NT(TBL_GBL_hhclosed).t = (float *)(calloc(11026, 4));
   NT(TBL_GBL_hhclosed).dmult = 9.070295e-05F;
   NT(TBL_GBL_hhclosed).lmult = 1.000000e+00F;
   NT(TBL_GBL_hhclosed).tend = 11024;
   NT(TBL_GBL_hhclosed).n = NT(TBL_GBL_hhclosed).diff = 9.999093e-01F;
   sfile = fopen("ch.wav", "rb");
   if (sfile == NULL)
   epr(35,"perc.saol","table","Samplefile not found");
   for (i = 0; i < 44; i++)
   if (!rread(hhclosed__sym_c,1,1,sfile))
   epr(35,"perc.saol","table","Corrupt samplefile");
   for (i=0; i < 11025; i++)
   {
   if (!rread(hhclosed__sym_c,1,2,sfile))
   epr(35,"perc.saol","table","Corrupt samplefile");
        hhclosed__sym_word = (short)hhclosed__sym_c[0];
        hhclosed__sym_word |= (short)hhclosed__sym_c[1]<<8;
        NT(TBL_GBL_hhclosed).t[i] = 3.051758e-5F*hhclosed__sym_word;
   }
    fclose(sfile);
   NT(TBL_GBL_hhclosed).t[11025] = NT(TBL_GBL_hhclosed).t[0];
   NT(TBL_GBL_times).lenf = (float)(NT(TBL_GBL_times).len = 17);
   NT(TBL_GBL_times).tend = 16;
   NT(TBL_GBL_times).n = NT(TBL_GBL_times).diff = 0.941176;
   NT(TBL_GBL_times).t = (float *)(table_global_times);
   NT(TBL_GBL_midimap).lenf = (float)(NT(TBL_GBL_midimap).len = 128);
   NT(TBL_GBL_midimap).tend = 127;
   NT(TBL_GBL_midimap).n = NT(TBL_GBL_midimap).diff = 0.992188;
   NT(TBL_GBL_midimap).t = (float *)(table_global_midimap);
   NT(TBL_GBL_azimuth).lenf = (float)(NT(TBL_GBL_azimuth).len = 17);
   NT(TBL_GBL_azimuth).tend = 16;
   NT(TBL_GBL_azimuth).n = NT(TBL_GBL_azimuth).diff = 0.941176;
   NT(TBL_GBL_azimuth).t = (float *)(table_global_azimuth);
   NT(TBL_GBL_elevation).lenf = (float)(NT(TBL_GBL_elevation).len = 17);
   NT(TBL_GBL_elevation).tend = 16;
   NT(TBL_GBL_elevation).n = NT(TBL_GBL_elevation).diff = 0.941176;
   NT(TBL_GBL_elevation).t = (float *)(table_global_elevation);
   NT(TBL_GBL_levels).lenf = (float)(NT(TBL_GBL_levels).len = 17);
   NT(TBL_GBL_levels).tend = 16;
   NT(TBL_GBL_levels).n = NT(TBL_GBL_levels).diff = 0.941176;
   NT(TBL_GBL_levels).t = (float *)(table_global_levels);
   NT(TBL_GBL_string_qscale).lenf = (float)(NT(TBL_GBL_string_qscale).len = 128);
   NT(TBL_GBL_string_qscale).tend = 127;
   NT(TBL_GBL_string_qscale).n = NT(TBL_GBL_string_qscale).diff = 0.992188;
   NT(TBL_GBL_string_qscale).t = (float *)(table_global_string_qscale);
   NT(TBL_GBL_string_gscale).lenf = (float)(NT(TBL_GBL_string_gscale).len = 128);
   NT(TBL_GBL_string_gscale).tend = 127;
   NT(TBL_GBL_string_gscale).n = NT(TBL_GBL_string_gscale).diff = 0.992188;
   NT(TBL_GBL_string_gscale).t = (float *)(table_global_string_gscale);
   NT(TBL_GBL_string_vel).lenf = (float)(NT(TBL_GBL_string_vel).len = 128);
   NT(TBL_GBL_string_vel).tend = 127;
   NT(TBL_GBL_string_vel).n = NT(TBL_GBL_string_vel).diff = 0.992188;
   NT(TBL_GBL_string_vel).t = (float *)(table_global_string_vel);

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

e_rv[0].noteon = TOBEPLAYED;
e_rv[0].starttime = 0.0F;
e_rv[0].abstime = 0.0F;
e_rv[0].released = 0;
e_rv[0].turnoff = 0;
e_rv[0].time = 0.0F;
e_rv[0].itime = 0.0F;
e_rv[0].sdur = -1.0F;
e_rv[0].kbirth = kbase;
e_rv[0].numchan = midimasterchannel;


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

 for (sysidx=cm_yaxupaxo___first;sysidx<=cm_yaxupaxo___last;sysidx++)
  if (sysidx->noteon == PLAYING)
   yaxupaxo_apass(sysidx->nstate);
 for (sysidx=d_string_audio___first;sysidx<=d_string_audio___last;sysidx++)
  if ((sysidx->noteon == PLAYING) &&(sysidx->launch == LAUNCHED))
   string_audio_apass(sysidx->nstate);
 for (sysidx=cm_perc___first;sysidx<=cm_perc___last;sysidx++)
  if (sysidx->noteon == PLAYING)
   perc_apass(sysidx->nstate);
   rv1_apass();

}

int main_kpass(void)

{

 for (sysidx=cm_yaxupaxo___first;sysidx<=cm_yaxupaxo___last;sysidx++)
  if (sysidx->noteon == PLAYING)
  {
   yaxupaxo_kpass(sysidx->nstate);
  }
 for (sysidx=cm_string_kbd___first;sysidx<=cm_string_kbd___last;sysidx++)
  if (sysidx->noteon == PLAYING)
  {
   string_kbd_kpass(sysidx->nstate);
  }
 for (sysidx=d_string_audio___first;sysidx<=d_string_audio___last;sysidx++)
  if (sysidx->noteon == PLAYING)
  {
   sysidx->launch = LAUNCHED;
   string_audio_kpass(sysidx->nstate);
  }
 for (sysidx=cm_perc___first;sysidx<=cm_perc___last;sysidx++)
  if (sysidx->noteon == PLAYING)
  {
   perc_kpass(sysidx->nstate);
  }
   rv1_kpass();

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
        case CSYS_MIDI_CC_RESETALLCONTROL:
         for (i = 0; i < CSYS_MIDI_CC_ALLSOUNDOFF; i++)
           NG(CSYS_FRAMELEN*(ec+CSYS_EXTCHANSTART)+CSYS_CCPOS+i)=0.0F;
         NG(CSYS_FRAMELEN*(ec + CSYS_EXTCHANSTART)+
            CSYS_MIDI_CC_CHANVOLUME_MSB) = 100.0F;
         NG(CSYS_FRAMELEN*(ec + CSYS_EXTCHANSTART)+
            CSYS_MIDI_CC_PAN_MSB) = 64.0F;
         NG(CSYS_FRAMELEN*(ec + CSYS_EXTCHANSTART)+
            CSYS_MIDI_CC_EXPRESSION_MSB) = 127.0F;
         csys_bank = csys_banklsb = csys_bankmsb = 0;
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

 for (sysidx=cm_yaxupaxo___first;sysidx<=cm_yaxupaxo___last;sysidx++)
  {
  switch(sysidx->noteon) {
   case PLAYING:
   if (sysidx->released)
    {
     if (sysidx->turnoff)
      {
        sysidx->noteon = ALLDONE;
        for (i = 0; i < yaxupaxo_ENDTBL; i++)
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
           for (i = 0; i < yaxupaxo_ENDTBL; i++)
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
      yaxupaxo_ipass(sysidx->nstate);
    }
   break;
   default:
   break;
   }
 }
  while (cm_yaxupaxo___last->noteon == ALLDONE)
   if (cm_yaxupaxo___last == cm_yaxupaxo___first)
    {
     cm_yaxupaxo___first = &cm_yaxupaxo[1];
     cm_yaxupaxo___last =  &cm_yaxupaxo[0];
     cm_yaxupaxo___next =  NULL;
     cm_yaxupaxo___last->noteon = TOBEPLAYED;
     break;
    }
   else
    cm_yaxupaxo___last--;
 for (sysidx=cm_string_kbd___first;sysidx<=cm_string_kbd___last;sysidx++)
  {
  switch(sysidx->noteon) {
   case PLAYING:
   if (sysidx->released)
    {
     if (sysidx->turnoff)
      {
        sysidx->noteon = ALLDONE;
        for (i = 0; i < string_kbd_ENDTBL; i++)
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
           for (i = 0; i < string_kbd_ENDTBL; i++)
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
      string_kbd_ipass(sysidx->nstate);
    }
   break;
   default:
   break;
   }
 }
  while (cm_string_kbd___last->noteon == ALLDONE)
   if (cm_string_kbd___last == cm_string_kbd___first)
    {
     cm_string_kbd___first = &cm_string_kbd[1];
     cm_string_kbd___last =  &cm_string_kbd[0];
     cm_string_kbd___next =  NULL;
     cm_string_kbd___last->noteon = TOBEPLAYED;
     break;
    }
   else
    cm_string_kbd___last--;
 for (sysidx=d_string_audio___first;sysidx<=d_string_audio___last;sysidx++)
  {
  switch(sysidx->noteon) {
   case PLAYING:
   if (sysidx->released)
    {
     if (sysidx->turnoff)
      {
        sysidx->noteon = ALLDONE;
        for (i = 0; i < string_audio_ENDTBL; i++)
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
           for (i = 0; i < string_audio_ENDTBL; i++)
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
        if (sysidx->endtime <= scorebeats)
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
      sysidx->numchan = midimasterchannel;
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
      string_audio_ipass(sysidx->nstate);
    }
   break;
   case PAUSED:
      sysidx->noteon = PLAYING;
      sysidx->kbirth = kcycleidx;
   break;
   default:
   break;
   }
 }
  while (d_string_audio___last->noteon == ALLDONE)
   if (d_string_audio___last == d_string_audio___first)
    {
     d_string_audio___first = &d_string_audio[1];
     d_string_audio___last =  &d_string_audio[0];
     d_string_audio___next =  NULL;
     d_string_audio___last->noteon = TOBEPLAYED;
     break;
    }
   else
    d_string_audio___last--;
 for (sysidx=cm_perc___first;sysidx<=cm_perc___last;sysidx++)
  {
  switch(sysidx->noteon) {
   case PLAYING:
   if (sysidx->released)
    {
     if (sysidx->turnoff)
      {
        sysidx->noteon = ALLDONE;
        for (i = 0; i < perc_ENDTBL; i++)
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
           for (i = 0; i < perc_ENDTBL; i++)
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
      perc_ipass(sysidx->nstate);
    }
   break;
   default:
   break;
   }
 }
  while (cm_perc___last->noteon == ALLDONE)
   if (cm_perc___last == cm_perc___first)
    {
     cm_perc___first = &cm_perc[1];
     cm_perc___last =  &cm_perc[0];
     cm_perc___next =  NULL;
     cm_perc___last->noteon = TOBEPLAYED;
     break;
    }
   else
    cm_perc___last--;

}

void main_initpass(void)

{

  e_rv[0].noteon = PLAYING;
  e_rv[0].notestate = nextstate;
  e_rv[0].endtime = MAXENDTIME;
  e_rv[0].nstate = &ninstr[nextstate];
  ninstr[nextstate].iline = &e_rv[0];
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
   rv1_ipass();


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





