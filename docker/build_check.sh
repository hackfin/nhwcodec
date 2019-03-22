#!/bin/bash

cd $HOME/src/nhwcodec; make clean all

cd $HOME/src/nhwcodec/test; make all-valgrind
