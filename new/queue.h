#include <lo/lo.h>


typedef struct {
  float time;
  lo_arg **argv;
  void *next;
} queue_event;
