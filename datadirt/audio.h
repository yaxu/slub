
#define SAMPLERATE        44100

#define MAXHEADS          30
#define MAXGROUPS         16
#define MAXSAMPLES        8024
#define MAXPATHSIZE       255

#define BUFFERSIZE        (128)

#define COMPRESSORGAIN    200

#define MAXLINE           16248

/* #define LATENCY */

float compressor;

#ifdef USE_JACK

#define JACK_GROUP_PREFIX    "start_"

typedef struct {
  char *name;
  jack_port_t *left_port;
  jack_port_t *right_port;
} t_group;

/*  Globals for groups and listeners */
t_group *groups[MAXGROUPS];
int group_count;

int init_jack_group(char *name);
/* t_group *add_group(char *group_name); */
/* int group_exists(char *group_name); */

int process_jack_out(jack_nframes_t nframes, void *arg);
int process_jack_in(jack_nframes_t nframes, void *arg);

#endif

/* structs */

typedef struct {
  float l;
  float r; 
} stereo;


#ifdef LATENCY
stereo delayline[LATENCY];
int delaypoint;
#endif

typedef struct {
  float *values;
  int size;
} t_table;

typedef struct {
  float *values;
  int size;
  t_table *table;
} t_envelope;

typedef struct {
  char name[MAXPATHSIZE];
  SF_INFO *info;
  float *frames;
} t_sample;

typedef struct {
 float cutoff;
 float res;
 float f;
 float k;
 float p;
 float scale;
 float r;
 float y1;
 float y2;
 float y3;
 float y4;
 float oldx;
 float oldy1;
 float oldy2;
 float oldy3;
 float x;
} t_vcf;

typedef struct {
  t_sample *sample;
  float  duration;
  float  position;
  float  speed;
  short  int active;
  t_envelope *env_volume;
  float  shape;
  float  pan;
  float  pan_to;
  float  volume;
  float  anafeel_strength;
  float  anafeel_frequency;
  float  accellerate;
  double formant_history[2][10];
  int    formant_vowelnum;
  t_vcf  vcf[2];
  int    foo;
  float  delay;
  float  resonance;
  float  cutoff;
} t_head;



t_head heads[MAXHEADS];
int head_count;

typedef struct {
  float samples[MAXLINE];
  int   point;
} t_line;

t_line *line_l;
t_line *line_r;
float line_feedback_delay;
t_line *line_l2;
t_line *line_r2;
float line_feedback_delay2;

t_sample *samples[MAXSAMPLES];
int sample_count;

/* envelopes */

t_envelope *env_simple;
t_envelope *env_tri;
t_envelope *env_percussive;
t_envelope *env_valley;
t_envelope *env_slope;
t_envelope *env_chop;
t_envelope *env_none;

float scale_equal[12] =
  {1.00000, 1.05946, 1.12246, 1.18921, 1.25992, 
   1.33483, 1.41421, 1.49831, 1.58740, 1.68179,
   1.78180, 1.88775
  };

