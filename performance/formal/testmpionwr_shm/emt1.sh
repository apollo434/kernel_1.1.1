#!/bin/bash

echo $1 $2 
taskset -c $1 ./emt1 $1 4000000 60000
