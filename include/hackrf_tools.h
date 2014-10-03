#ifndef HACKRF_TOOLS
#define HACKRF_TOOLS

#include <zmq.h>
#include <zmq.hpp>
#include <string>
#include <vector>
#include <complex.h>
#include <cstdlib>
#include <ostream>
#include "zhelpers.hpp"


typedef enum _Axes {x, y} Axes;

class Data_Publisher {
   public:
      Data_Publisher(std::string channel);
      ~Data_Publisher();

      void Send(std::vector<float>* data, Axes axes = y);
      void Send(std::vector<complex>* data, Axes axes = y);
      void Serialize();

   private:

      zmq::context_t* ctx;
      zmq::socket_t* sock;
      int port;
      std::string host_addr;
      std::string endpoint;
      std::string channel;

   protected:

};

#endif
