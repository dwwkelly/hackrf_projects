#!/bin/bash

protoc --proto_path=proto --cpp_out=src --python_out=src proto/realtime.proto
mv src/realtime.pb.h include
