#!/usr/bin/env python

__author__ = "Devin Kelly"

import zmq
import time
import realtime_pb2
import numpy as np


def main():

   ctx = zmq.Context(1)
   publisher = ctx.socket(zmq.PUB)
   # publisher.connect("tcp://localhost:5563")
   publisher.bind("tcp://*:5500")

   data = realtime_pb2.Data()
   data.length = 25
   data.axis = data.x_axis
   data.samples.extend(np.arange(1, 26, 1).tolist())

   serialized = data.SerializeToString()

   publisher.send_multipart([b'A', serialized])

   for ii in xrange(50):
      data = realtime_pb2.Data()
      data.length = 25
      data.samples.extend(np.random.random_sample(data.length).tolist())
      data.axis = data.y_axis
      serialized = data.SerializeToString()
      publisher.send_multipart([b'A', serialized])
      time.sleep(1)

   return

if __name__ == "__main__":
   main()
