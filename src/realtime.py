#!/usr/bin/env python

__author__ = "Devin Kelly"


import zmq
import numpy as np
import time
import realtime_pb2
from matplotlib.pylab import subplots, close
from matplotlib import cm
import matplotlib.pyplot as plt


def run():
   """
   Visualise the simulation using matplotlib, using blit for
   improved speed
   """

   # setup zmq
   ctx = zmq.Context(1)
   subscriber = ctx.socket(zmq.SUB)
   subscriber.bind("tcp://*:5501")
   subscriber.setsockopt(zmq.SUBSCRIBE, "")
   time.sleep(1)

   # setup plot
   fig = plt.figure()
   fig.show()
   ax = plt.gca()
   ax.set_xlim(0, 5)
   ax.set_ylim(0, 5)
   ax.hold(True)
   fig.canvas.draw()

   background = fig.canvas.copy_from_bbox(ax.bbox)
   x = [0]
   y = [0]
   plot = ax.plot(x, y)[0]

   while(True):

      [address, contents] = subscriber.recv_multipart()

      msg = realtime_pb2.Data()
      msg.ParseFromString(contents)

      if msg.axis == realtime_pb2.Data.x_axis:
         x = np.array(msg.samples)
      elif msg.axis == realtime_pb2.Data.y_axis:
         y = np.array(msg.samples)

      # update the xy data
      if len(x) == len(y):
         plot.set_data(x, y)

         # restore background
         fig.canvas.restore_region(background)

         # redraw just the points
         ax.draw_artist(plot)

         # fill in the axes rectangle
         fig.canvas.blit(ax.bbox)

   close(fig)
   subscriber.close()
   context.term()


def main():

   run()

   return

if __name__ == "__main__":
   main()
