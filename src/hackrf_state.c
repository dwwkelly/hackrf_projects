#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include "hackrf_state.h"
#include "util.h"

int init_state(State* s)
{
   int rc;
   char* default_file = "/tmp/data";
   size_t default_file_len = strnlen(default_file, MAXFILENAMELEN);
   s->fc = 98000000;  // 98 MHz
   s->fs = 20000000; // 20 MHz
   s->lna_gain = 0;
   s->vga_gain = 0;
   s->samples_needed = 10000;
   s->filename = NULL;
   change_filename(s, default_file, default_file_len);
   s->fd = fopen(s->filename, "w");

   rc = pthread_mutex_init(&(s->lock), NULL);
   CHECK_PTHREAD(rc)

   rc = pthread_cond_init(&(s->cond), NULL);
   CHECK_PTHREAD(rc)

   rc = pthread_mutex_lock(&(s->lock));
   CHECK_PTHREAD(rc)

   s->lut = (complex float*) malloc(sizeof(complex float) * 65536);
   unsigned int i;
   for ( i = 0; i <= 0xffff; i++) {
      float re = (float) ((char)(i & 0xff)) * 1.0f/128.0f;
      float im = (float) ((char)(i >> 8)) * 1.0f/128.0f;
      complex float tmp = re + im * _Complex_I;
      s->lut[i] = tmp;
   }

   return 0;
}

int print_state(State* s, FILE* dest)
{
   fprintf(dest, "Center Frequency: %lu\n", s->fc);
   fprintf(dest, "Sample Rate: %d\n", s->fs);
   fprintf(dest, "Number of Samples: %d\n", s->samples_needed);
   fprintf(dest, "VGA Gain: %d\n", s->vga_gain);
   fprintf(dest, "LNA Gain: %d\n", s->lna_gain);
   fprintf(dest, "Filename: %s\n", s->filename);
   fprintf(dest, "Filename Length: %lu\n", s->filename_len);
   return 0;
}

int print_state_json(State* s, FILE* dest)
{
   const char* fmt = "{\"fc\":%lu, \"fs\":%lu, \"n\":%d, \"vga gain\": %d, \"lna gain\":%d}\n";
   fprintf(dest, fmt, s->fc, s->fs, s->samples_needed, s->vga_gain, s->lna_gain);
   return 0;
}

int change_filename(State* s, char* new_filename, size_t new_filename_len)
{
   if(s->filename != NULL) {
      free((void*)s->filename);
      s->filename = NULL;
   }

   size_t malloc_size = sizeof(char) * (new_filename_len + 1);
   s->filename = (char*) malloc(malloc_size);
   CHECK_MALLOC(s->filename)
   strncpy(s->filename, new_filename, malloc_size);
   s->filename_len = new_filename_len;

   if(s->fd != NULL) {
      fclose(s->fd);
   }
   s->fd = fopen(s->filename, "w");

   return 0;
}

int free_state(State* s)
{

   if(s->fd != NULL){
      fclose(s->fd);
   }

   if(s->filename != NULL) {
      free((void*)s->filename);
      s->filename = NULL;
   }

   if(s != NULL){
      free((void*)s);
      s = NULL;
   }
   return 0;
}
