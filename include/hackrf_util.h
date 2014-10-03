#ifndef HACKRF_UTIL
#define HACKRF_UTIL

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

#define HACKRF_ERROR_CHECK(rc) \
   if(rc != HACKRF_SUCCESS) { \
      printf("%s (%d)\n", hackrf_error_name((enum hackrf_error)rc), rc); \
      return -1; \
   }

#define CHECK_MALLOC(ptr) \
   if(ptr == NULL) { \
      printf("malloc() failed: %s\n", strerror(errno)); \
      exit(-1); \
   }

#define CHECK_PTHREAD(rc) \
   if(rc != 0) { \
      printf("%s\n", strerror(rc)); \
      exit(EXIT_FAILURE); \
   }

#define CHECK_ZMQ(rc) \
   if(rc < 0) { \
      printf("%s\n", zmq_strerror(zmq_errno())); \
      exit(EXIT_FAILURE); \
   }

#define CHECK_ZMQ_PTR(ptr) \
   if(ptr == NULL) { \
      printf("%s\n", zmq_strerror(zmq_errno())); \
      exit(EXIT_FAILURE); \
   }


int read_samples_from_file(const char* filename, complex double* samples, size_t samples_len);
int write_samples_to_file(const char* filename, complex double* samples, size_t samples_len);
int print_samples(complex double* samples, size_t samples_len);

#endif
