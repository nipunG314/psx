#!/bin/bash
set -x

bear make
./src/psx "$@" 
