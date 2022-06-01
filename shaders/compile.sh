#!/bin/bash

dir=`dirname $0`
glslc $dir/shader.vert -o $1/vert.spv
glslc $dir/shader.frag -o $1/frag.spv
