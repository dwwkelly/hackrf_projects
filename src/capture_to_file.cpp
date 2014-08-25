#include <libhackrf/hackrf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#define HACKRF_ERROR_CHECK(rc) \
   if(rc != HACKRF_SUCCESS) { \
      printf("%s (%d)\n", hackrf_error_name((hackrf_error)rc), rc); \
      return -1; \
   }

#define CHECK_MALLOC(ptr) \
   if(ptr == NULL) { \
      printf("malloc() failed: %s\n", strerror(errno)); \
      exit(-1); \
   }

#define MAXFILENAMELEN 256

typedef struct _state {
   int counter;
   uint64_t fc;
   uint32_t fs;
   uint32_t lna_gain;
   uint32_t vga_gain;
   uint32_t samples_needed;
   uint32_t samples_collected;
   char* filename;
   size_t filename_len;
   FILE* fd;
} State;

int hackrf_rx_callback(hackrf_transfer *transfer);

int init_state(State* s);
int print_state(State* s, FILE* dest);
int free_state(State* s);
int get_args(int argc, char* const argv[], State* s);
int usage();
int change_filename(State* s, char* new_filename, size_t new_filename_len);

int get_args(int argc, char* const argv[], State* s) {
   const char* optstring = "hdf:F:l:v:s:n:";
   char n;
   while((n=getopt(argc, argv, optstring)) != -1) {
      switch(n){
         case 'f':  // Center Frequency
            s->fc = atoi(optarg);
            break;
         case 'F':  // Filename
            change_filename(s, optarg, strnlen(optarg, MAXFILENAMELEN));
            break;
         case 's':  // Sample Rate
            s->fs = atoi(optarg);
            break;
         case 'l':  // LNA gain
            s->lna_gain = atoi(optarg);
            break;
         case 'v':  // VGA gain
            s->vga_gain = atoi(optarg);
            break;
         case 'n':  // Number of samples
            s->samples_needed = atoi(optarg);
            break;
         case 'd':
            break;
         case 'h':
            usage();
            free_state(s);
            exit(0);
         default:
            usage();
            free_state(s);
            exit(0);
      }
   }

   optind = 1;
   while((n=getopt(argc, argv, optstring)) != -1) {
      if (n == 'd') {
         print_state(s, stdout);
         free_state(s);
         exit(0);
      }
   }

   return 0;
}

int usage()
{
   printf("[-f] Center Frequency    [default=98 MHz]\n");
   printf("[-s] Sample Rate         [default=20 MHz]\n");
   printf("[-n] Number of Samples   [default=5000]\n");
   printf("[-v] VGA Gain            [default=0 dB]\n");
   printf("[-l] LNA Gain            [default=0 dB]\n");
   printf("[-F] Filename            [default=/tmp/data]\n");
   printf("[-h] This message\n");
   return 0;
}

int init_state(State* s)
{
   char* default_file = "/tmp/data";
   size_t default_file_len = strnlen(default_file, MAXFILENAMELEN);
   s->counter = 0;
   s->fc = 98000000;  // 98 MHz
   s->fs = 20000000; // 20 MHz
   s->lna_gain = 0;
   s->vga_gain = 0;
   s->samples_needed = 10000;
   s->filename = NULL;
   change_filename(s, default_file, default_file_len);
   s->fd = fopen(s->filename, "w");
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

int change_filename(State* s, char* new_filename, size_t new_filename_len)
{
   if(s->filename != NULL) {
      printf("freeing\n");
      free((void*)s->filename);
      s->filename = NULL;
   }

   size_t malloc_size = sizeof(char) * (new_filename_len + 1);
   s->filename = (char*) malloc(malloc_size);
   CHECK_MALLOC(s->filename)
   strncpy(s->filename, new_filename, malloc_size);
   s->filename_len = new_filename_len;

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

int main(int argc, char *argv[])
{

   hackrf_device* device;
   int rc = HACKRF_SUCCESS;

   State* s = (State*) malloc(sizeof(State));
   CHECK_MALLOC(s)

   init_state(s);

   get_args(argc, argv, s);

   /* initialization */
   rc = hackrf_init();
   HACKRF_ERROR_CHECK(rc)

   /* open hack rf */
   rc = hackrf_open(&device);
   HACKRF_ERROR_CHECK(rc)

   /* set sample rate */
   rc = hackrf_set_sample_rate_manual(device, s->fs, 1);
   HACKRF_ERROR_CHECK(rc)

   /* set gain */
   rc = hackrf_set_lna_gain(device, s->lna_gain);
   HACKRF_ERROR_CHECK(rc)

   rc = hackrf_set_vga_gain(device, s->vga_gain);
   HACKRF_ERROR_CHECK(rc)

   /* tune */
   rc = hackrf_set_freq(device, s->fc);
   HACKRF_ERROR_CHECK(rc)

   /* start hacking */
   rc = hackrf_start_rx(device, hackrf_rx_callback, (void*)s);
   HACKRF_ERROR_CHECK(rc)

   sleep(100);

   return 0;
}

int hackrf_rx_callback(hackrf_transfer *transfer)
{

   int rc;
   State* s = (State*) transfer->rx_ctx;
   printf("callback: %d\n", s->counter);
   s->counter = s->counter + 1;
   FILE* fd = s->fd;

   unsigned char* buf = transfer->buffer;
   uint32_t size = transfer->valid_length;
   s->samples_collected += size/2;

   uint32_t ii;
   for(ii=0; ii<(size/2); ii++){
      float I = (float(char(buf[2 * ii]))) * 1.0f / 128.0f;
      float Q = (float(char(buf[2 * ii + 1]))) * 1.0f / 128.0f;
      fprintf(fd, "%f,%f\n", I, Q);
   }

   if(s->samples_collected >= s->samples_needed){
      printf("stopping\n");
      fclose(s->fd);
      rc = hackrf_stop_rx(transfer->device);
      HACKRF_ERROR_CHECK(rc)
   }

   return 0;
}
