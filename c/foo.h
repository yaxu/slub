#define MAXGROUPS         1024
#define MAXLISTENERS      1024
/* JACK related constants */
#define JACK_GROUP_PREFIX    "foo_"
#define JACK_LISTENER_PREFIX "foo_ear_"


/* typedefs */
typedef struct {
  char *name;
  jack_port_t *left_port;
  jack_port_t *right_port;
} t_group;

/*  Globals for groups and listeners */
t_group *groups[MAXGROUPS];
int group_count;

t_listener *listeners[MAXLISTENERS];
int listener_count;

/* stereo struct for easy handling of stereo sample pairs */

typedef struct {
  float l;
  float r; 
  float mono; /*  optionally used */
} stereo;

typedef struct {
  char *name;
  jack_port_t *mono_port;
  int last_write;
} t_listener;

