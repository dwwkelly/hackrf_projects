#include "hackrf_tools.h"
#include "unistd.h"
#include "realtime.pb.h"

Data_Publisher::Data_Publisher(std::string channel) {
   GOOGLE_PROTOBUF_VERIFY_VERSION;

   channel = channel;
   port = 5500;
   host_addr = "localhost";
   endpoint = "tcp://";
   endpoint.append(host_addr);
   endpoint.append(":");
   std::ostringstream ss;
   ss << port;
   endpoint.append(ss.str());

   ctx = new zmq::context_t(1);
   sock = new zmq::socket_t(*ctx, ZMQ_PUB);
   sock->connect("tcp://localhost:5501");

   sleep(1);

}

Data_Publisher::~Data_Publisher() {

}

void Data_Publisher::Send(std::vector<float>* data, Axes axes) {
   Data* d = new Data();

   d->mutable_samples()->Reserve(data->size());
   for(int ii=0; ii<data->size(); ii++)
      d->add_samples(data->at(ii));

   d->set_length(data->size());
   if (axes == x) {
      d->set_axis(d->x_axis);
   } else {
      d->set_axis(d->y_axis);
   }

   std::string s;
   d->SerializeToString(&s);

   s_sendmore(*sock, "A");
   zmq::message_t message(s.size());
   memcpy (message.data(), s.data(), s.size());
   sock->send(message);

}

void Data_Publisher::Send(std::vector<complex>* data, Axes axes) {

}

void Data_Publisher::Serialize() {

}
