
#define MAXHEADS          60
#define MAXGROUPS         16
#define MAXSAMPLES        8024
#define MAXPATHSIZE       255

#define SAMPLEROOT        "/yaxu/samples"
#define BUFFERSIZE        1024

#define COMPRESSORGAIN    100

float compressor;

/* JACK related constants */
#define JACK_GROUP_PREFIX    "start_"


/* structs */

typedef struct {
  float l;
  float r; 
} stereo;

typedef struct {
  char *name;
  jack_port_t *left_port;
  jack_port_t *right_port;
} t_group;

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
  t_sample *sample;
  float position;
  float speed;
  short int active;
  t_envelope *env_volume;
  float shape;
  float pan;
  float volume;
} t_head;

t_head heads[MAXHEADS];
int head_count;

t_sample *samples[MAXSAMPLES];
int sample_count;

/*  Globals for groups and listeners */
t_group *groups[MAXGROUPS];
int group_count;

/* envelopes */

t_envelope *env_tri;
t_envelope *env_percussive;
t_envelope *env_valley;

int init_jack_group(char *name);
/* t_group *add_group(char *group_name); */
/* int group_exists(char *group_name); */

int process_jack_out(jack_nframes_t nframes, void *arg);
int process_jack_in(jack_nframes_t nframes, void *arg);


