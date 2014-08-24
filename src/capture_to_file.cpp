#include <libhackrf/hackrf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define HACKRF_ERROR_CHECK(rc) \
   if(rc != HACKRF_SUCCESS) { \
      printf("%s (%d)\n", hackrf_error_name((hackrf_error)rc), rc); \
      return -1; \
   }

typedef struct _state {
   int counter;
   uint64_t fc;
   uint32_t fs;
   uint32_t lna_gain;
   uint32_t vga_gain;
   char* filename;
   size_t filename_len;
   FILE* fd;
} State;

int hackrf_rx_callback(hackrf_transfer *transfer);

int main(int argc, char *argv[])
{

   hackrf_device* device;
   uint8_t board_id = BOARD_ID_INVALID;
   int rc = HACKRF_SUCCESS;

   State* s = (State*) malloc(sizeof(State));
   s->counter = 0;
   s->fc = 440000000;  //108000000;
   s->lna_gain = 0;
   s->vga_gain = 0;
   s->fs = 20000000;
   s->filename = "/tmp/data";
   s->filename_len = strnlen(s->filename, 256);
   s->fd = fopen(s->filename, "ab");

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

   int ii;
   for(ii=0; ii<(size/2); ii++){
      float I = (float(char(buf[2 * ii]))) * 1.0f / 128.0f;
      float Q = (float(char(buf[2 * ii + 1]))) * 1.0f / 128.0f;
      fprintf(fd, "%f,%f\n", I, Q);
   }

   if(s->counter >= 10){
      printf("stopping\n");
      fclose(s->fd);
      rc = hackrf_stop_rx(transfer->device);
      HACKRF_ERROR_CHECK(rc)
   }

   return 0;
}
