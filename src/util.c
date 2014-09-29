#include "util.h"

int read_samples_from_file(const char* filename, complex double* samples, size_t samples_len) {

   if(filename == NULL || samples == NULL) {
      return -1;
   }

   FILE* fd = fopen(filename, "r");
   if(fd == NULL) {
      return 1;
   }

   char * line = NULL;
   size_t len = 0;
   ssize_t read = 0;
   size_t counter = 0;
   double tmp = 0.0;

   while((read == getline(&line, &len, fd)) != -1){
      sscanf(line, "%lf", &tmp);
      *(samples + counter) = tmp + 0.0f * I;

      counter++;

      if(counter >= samples_len) {
         free(line);
         fclose(fd);
         return 1;
      }

   }

   free(line);
   fclose(fd);
   return 0;
}

int write_samples_to_file(const char* filename, complex double* samples, size_t samples_len) {

   if(filename == NULL || samples == NULL) {
      return 1;
   }

   FILE* fd = fopen(filename, "w");
   if(fd == NULL) {
      return 1;
   }

   size_t ii = 0;
   for(ii=0; ii<samples_len; ii++) {
      fprintf(fd, "%lf,%lf\n", creal(samples[ii]), cimag(samples[ii]));
   }

   fclose(fd);
   return 0;
}

int print_samples(complex double* samples, size_t samples_len) {

   size_t ii = 0;
   for(ii=0; ii<samples_len; ii++) {
      printf("%lf + %lf * I\n", creal(samples[ii]), cimag(samples[ii]));
   }

   return 0;

}
