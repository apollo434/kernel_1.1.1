#!/bin/bash

#echo $1 $2 
taskset -c $1 ./emtio $1 40000 60
