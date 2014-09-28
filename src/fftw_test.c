#ifndef _GNU_SOURCE
   #define _GNU_SOURCE
#endif

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#include <complex.h>
#include <fftw3.h>


int read_samples(const char* filename, complex double* samples, size_t samples_len);
int write_samples(const char* filename, complex double* samples, size_t samples_len);
int print_samples(complex double* samples, size_t samples_len);

int main()
{

   //int length = 10000;
   int length = 1024;
   fftw_plan p;
   complex double* x = (complex double*) malloc(sizeof(complex double) * length);
   complex double* y = (complex double*) malloc(sizeof(complex double) * length);
   complex double* Y = (complex double*) malloc(sizeof(complex double) * length);
   p = fftw_plan_dft_1d(length, y, Y, FFTW_FORWARD, FFTW_ESTIMATE);

   read_samples("/tmp/x", x, length);
   read_samples("/tmp/y", y, length);

   fftw_execute(p);

   write_samples("/tmp/Y", Y, length);

   fftw_destroy_plan(p);
   free(x);
   free(y);
   free(Y);

   return 0;
}

int read_samples(const char* filename, complex double* samples, size_t samples_len) {

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

int write_samples(const char* filename, complex double* samples, size_t samples_len) {

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
