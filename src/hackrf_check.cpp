#include <libhackrf/hackrf.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{

   hackrf_device* device;
   uint8_t board_id = BOARD_ID_INVALID;
   int rc = HACKRF_SUCCESS;

   /* initialization */
   rc = hackrf_init();
   if(rc != HACKRF_SUCCESS) {
      printf("%s (%d)\n", hackrf_error_name((hackrf_error)rc), rc);
      return -1;
   }

   /* open hack rf */
   rc = hackrf_open(&device);
   if(rc != HACKRF_SUCCESS) {
      printf("%s (%d)\n", hackrf_error_name((hackrf_error)rc), rc);
      return -1;
   }

   /* check board ID */
   rc = hackrf_board_id_read(device, &board_id);
   if (rc != HACKRF_SUCCESS) {
      printf("%s (%d)\n", hackrf_error_name((hackrf_error)rc), rc);
      return -1;
   }

   printf("Board ID Number: %d (%s)\n",
          board_id,
          hackrf_board_id_name((hackrf_board_id)board_id));

   rc = hackrf_close(device);
   if (rc != HACKRF_SUCCESS){
      printf("%s (%d)\n", hackrf_error_name((hackrf_error)rc), rc );
      return -1;
   }
   return 0;
}

