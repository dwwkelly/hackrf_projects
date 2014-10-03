#include <libhackrf/hackrf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <pthread.h>
#include <complex.h>

#include <hackrf_state.h>
#include <hackrf_util.h>

int hackrf_rx_callback(hackrf_transfer *transfer);

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
   printf("[-n] Number of Samples   [default=10000]\n");
   printf("[-v] VGA Gain            [default=0 dB]\n");
   printf("[-l] LNA Gain            [default=0 dB]\n");
   printf("[-F] Filename            [default=/tmp/data]\n");
   printf("[-h] This message\n");
   return 0;
}


int main(int argc, char *argv[])
{

   /* setup */

   hackrf_device* device;
   int rc = HACKRF_SUCCESS;

   State* s = (State*) malloc(sizeof(State));
   CHECK_MALLOC(s)
   init_state(s);

   get_args(argc, argv, s);

   print_state_json(s, s->fd);

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

   rc = pthread_cond_wait(&(s->cond), &(s->lock));
   if(rc < 0){
      printf("pthread_cond_wait() error\n");
      exit(EXIT_FAILURE);
   }

   exit(EXIT_SUCCESS);
}

int hackrf_rx_callback(hackrf_transfer *transfer)
{

   State* s = (State*) transfer->rx_ctx;
   if(s->samples_collected >= s->samples_needed){
      pthread_cond_signal(&(s->cond));
      return 0;
   }

   FILE* fd = s->fd;
   unsigned short* buf = (unsigned short*) transfer->buffer;
   uint32_t size = transfer->valid_length;

   if (size/2 > s->samples_needed - s->samples_collected) {
      size = (s->samples_needed - s->samples_collected);
   }
   s->samples_collected += size;

   unsigned short ii = 0;
   for(ii=0; ii<size; ii++){

      /*
      float I = (float(char(ii & 0xff))) * (1.0f/128.0f);
      float Q = (float(char(ii >> 8))) * (1.0f/128.0f);
      */

      complex float X = s->lut[buf[ii]];
      fprintf(fd, "%0.8f,%0.8f\n", crealf(X), cimagf(X));
   }

   return 0;
}
