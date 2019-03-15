#!/bin/sh
# Initialization script for NHW project
#

# Experimental branch:
GIT_URL=https://github.com/hackfin/nhwcodec.git
GIT_BRANCH=restruct

# Raphael Canuts stable branch
# GIT_URL=https://github.com/rcanut/nhwcodec.git
# GIT_BRANCH=master

cd $HOME/src && git clone $GIT_URL -b $GIT_BRANCH

WORKDIR=$HOME/work

# Get images
cd $WORKDIR && wget https://section5.ch/downloads/nhw_testimages.tgz && \
	tar xfz nhw_testimages.tgz

[ -e $WORKDIR/nhw/out ] || mkdir $WORKDIR/nhw/out
[ -e $WORKDIR/nhw/ref ] || mkdir $WORKDIR/nhw/ref
