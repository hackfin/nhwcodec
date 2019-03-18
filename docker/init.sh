#!/bin/sh
# Initialization script for NHW project
#

# Experimental branch:
GIT_URL=https://github.com/hackfin/nhwcodec.git
GIT_BRANCH=restruct

pushd .
# Raphael Canuts stable branch
# GIT_URL=https://github.com/rcanut/nhwcodec.git
# GIT_BRANCH=master

[ -e $HOME/src ] || mkdir $HOME/src
cd $HOME/src; [ -e nhwcodec ] || git clone $GIT_URL -b $GIT_BRANCH

WORKDIR=$HOME/work

# Get images
[ -e $WORKDIR ] || mkdir $WORKDIR
[ -e $WORKDIR/imgs.tgz ] || \
wget -O $WORKDIR/imgs.tgz https://section5.ch/downloads/nhw_testimages.tgz && \
	cd $WORKDIR && tar xvfz imgs.tgz

[ -e $WORKDIR/nhw/out ] || mkdir $WORKDIR/nhw/out
[ -e $WORKDIR/nhw/ref ] || mkdir $WORKDIR/nhw/ref
popd
