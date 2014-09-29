#include <libhackrf/hackrf.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <pthread.h>

#define MAXFILENAMELEN 256
#define MAXENDPOINTLEN 512


typedef struct _state {
   uint64_t fc;
   uint32_t fs;
   uint32_t lna_gain;
   uint32_t vga_gain;
   uint32_t samples_needed;
   uint32_t samples_collected;
   char* filename;
   size_t filename_len;
   FILE* fd;
   pthread_mutex_t lock;
   pthread_cond_t cond;
   complex float* lut;
} State;

int init_state(State* s);
int print_state(State* s, FILE* dest);
int print_state_json(State* s, FILE* dest);
int free_state(State* s);
int change_filename(State* s, char* new_filename, size_t new_filename_len);
