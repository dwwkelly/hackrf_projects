#include <libhackrf/hackrf.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <pthread.h>
#include <complex.h>
#include <fftw3.h>
#include <zmq.h>
#include "util.h"

#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"

#define MAXFILENAMELEN 256
#define MAXENDPOINTLEN 512

typedef struct _state {
   pthread_mutex_t state_lock;
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
   void* zmq_context;
   void* zmq_socket;
   char* zmq_endpoint;
   char* zmq_done_endpoint;
   void* zmq_done_socket;
} State;

int hackrf_rx_callback(hackrf_transfer *transfer);

int init_state(State* s);
int print_state(State* s, FILE* dest);
int print_state_json(State* s, FILE* dest);
int free_state(State* s);
int get_args(int argc, char* const argv[], State* s);
int usage();
int change_filename(State* s, char* new_filename, size_t new_filename_len);
void* fft(void* tmp);

int get_args(int argc, char* const argv[], State* s) {
   const char* optstring = "hdf:F:l:v:s:n:";
   char n;
   int len = 0;
   while((n=getopt(argc, argv, optstring)) != -1) {
      switch(n){
         case 'F':  // Filename
            len = strnlen(optarg, MAXFILENAMELEN);
            change_filename(s, optarg, len);
            break;
         case 'f':  // Center Frequency
            s->fc = atoi(optarg);
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
   s->zmq_socket = NULL;
   s->zmq_context = NULL;
   s->zmq_endpoint = NULL;

   rc = pthread_mutex_init(&(s->state_lock), NULL);
   CHECK_PTHREAD(rc)

   rc = pthread_mutex_init(&(s->lock), NULL);
   CHECK_PTHREAD(rc)

   rc = pthread_cond_init(&(s->cond), NULL);
   CHECK_PTHREAD(rc)

   rc = pthread_mutex_lock(&(s->lock));
   CHECK_PTHREAD(rc)

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

int main(int argc, char *argv[])
{

   /* setup */

   hackrf_device* device;
   int rc = HACKRF_SUCCESS;

   State* s = (State*) malloc(sizeof(State));
   CHECK_MALLOC(s)
   init_state(s);

   s->zmq_endpoint = "inproc://hackrf";
   s->zmq_done_endpoint = "inproc://done";
   s->zmq_context = zmq_ctx_new();
   CHECK_ZMQ_PTR(s->zmq_context)
   s->zmq_socket = zmq_socket(s->zmq_context, ZMQ_PUSH);
   CHECK_ZMQ_PTR(s->zmq_socket)

   s->zmq_done_socket = zmq_socket(s->zmq_context, ZMQ_PUB);
   CHECK_ZMQ_PTR(s->zmq_done_socket)
   rc = zmq_bind(s->zmq_done_socket, s->zmq_done_endpoint);
   CHECK_ZMQ(rc)

   pthread_t fft_tid;
   pthread_create(&fft_tid, NULL, fft, (void*)s);

   while((rc = zmq_connect(s->zmq_socket, s->zmq_endpoint)) != 0){
      struct timespec t;
      int msecs = 250;
      t.tv_sec = msecs / 1000;
      t.tv_nsec = (msecs % 1000) * 1000000;
      nanosleep(&t, NULL);
   }

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
   CHECK_PTHREAD(rc)

   rc = pthread_join(fft_tid, NULL);
   CHECK_PTHREAD(rc)

   exit(EXIT_SUCCESS);
}

int hackrf_rx_callback(hackrf_transfer *transfer)
{
   State* s = (State*) transfer->rx_ctx;
   if(s->samples_collected >= s->samples_needed){
      int rc = 0;
      rc = zmq_send(s->zmq_done_socket, "DONE", 4, ZMQ_SNDMORE);
      CHECK_ZMQ(rc)
      rc = zmq_send(s->zmq_done_socket, "DONE", 4, 0);
      CHECK_ZMQ(rc)
      pthread_cond_signal(&(s->cond));
      return 0;
   }

   int rc;

   unsigned char* buf = transfer->buffer;
   uint32_t size = transfer->valid_length;
   s->samples_collected += size/2;

   double complex *samples;
   samples = (complex double *) malloc(sizeof(complex double) * size/2);
   uint32_t counter = 0;

   uint32_t ii;
   for(ii=0; ii<(size/2); ii++){
      double i = (double) ((char)buf[2 * ii]);
      i *= 1.0f / 128.0f;
      double q = (double) ((char)(buf[2 * ii + 1]));
      q *= 1.0f / 128.0f;
      complex double x = i + q * I;
      samples[counter] = x;
      counter++;
   }

   // send samples
   rc = zmq_send(s->zmq_socket, &counter, sizeof(counter), ZMQ_SNDMORE);
   CHECK_ZMQ(rc)
   rc = zmq_send(s->zmq_socket, samples, counter * sizeof(samples), 0);
   CHECK_ZMQ(rc)

   free((void*)samples);
   samples = NULL;
   return 0;
}

void* fft(void* tmp){

   State* s = (State*) tmp;
   int rc;
   char buf[5];

   pthread_mutex_lock(&(s->state_lock));

   void* sock = zmq_socket(s->zmq_context, ZMQ_PULL);
   CHECK_ZMQ_PTR(sock)
   rc = zmq_bind(sock, s->zmq_endpoint);
   CHECK_ZMQ(rc)

   void* done_sock = zmq_socket(s->zmq_context, ZMQ_SUB);
   CHECK_ZMQ_PTR(done_sock)
   rc = zmq_connect(done_sock, s->zmq_done_endpoint);
   CHECK_ZMQ(rc)
   pthread_mutex_unlock(&(s->state_lock));

   rc = zmq_setsockopt(done_sock, ZMQ_SUBSCRIBE, "DONE", 4);
   CHECK_ZMQ(rc)

   zmq_pollitem_t items[] = {{sock, 0, ZMQ_POLLIN, 0},
                             {done_sock, 0, ZMQ_POLLIN, 0}};

   int N = 4096;
   int max_samples = 5e6;
   fftw_complex *in = NULL;
   fftw_complex *out = NULL;
   in = (complex double *) fftw_malloc(sizeof(complex double) * max_samples);
   out = (complex double *) fftw_malloc(sizeof(complex double) * max_samples);
   fftw_plan p;
   p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);


   while(1) {
      size_t buf_size = 4194304;

      rc = zmq_poll(items, 2, -1);
      CHECK_ZMQ(rc)

      if(items[0].revents & ZMQ_POLLIN) {
         uint32_t n_samples;

         // get the number of samples coming in
         rc = zmq_recv(sock, &n_samples, sizeof(n_samples), 0);
         CHECK_ZMQ(rc)

         // get the samples themselves
         rc = zmq_recv(sock, in, sizeof(complex double) * buf_size, 0);
         CHECK_ZMQ(rc)

         fftw_execute(p);

         write_samples_to_file("/tmp/samples", in, N);
         write_samples_to_file("/tmp/fft_out", out, N);
      }

      if (items[1].revents & ZMQ_POLLIN) {
         rc = zmq_recv(done_sock, buf, 5, 0);
         CHECK_ZMQ(rc)
         if(rc > 0){
            if(strncmp(buf, "DONE", 4) == 0) {
               break;
            }
         }
      }
   }

   fftw_destroy_plan(p);
   if(in != NULL){
      fftw_free(in);
   }
   zmq_close(sock);
   zmq_close(done_sock);

   return NULL;
}
