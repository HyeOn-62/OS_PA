#!/bin/bash

string=$(g++ "$1.c" -o $1)

if [ -n "$string" ]; then
	exit -1
else
	./$1
fi
