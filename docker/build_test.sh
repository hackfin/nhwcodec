GOLDEN_REFERENCE=8b301f47fad3d57f6eb2c1060cc5f4f2d4b3ddce

GIT_REPO=https://github.com/hackfin/nhwcodec.git

cd src && \
[ -e nhw_reference ] || \
	git clone $GIT_REPO nhw_reference
cd nhw_reference
git checkout restruct
git checkout $GOLDEN_REFERENCE
# Pick TEST suite from latest:
git checkout restruct -- test

cd $HOME/src/nhwcodec; make clean all
cd $HOME/src/nhw_reference; make clean all

cd $HOME/src/nhw_reference/test; make clean all-nhw reference
cd $HOME/src/nhwcodec/test; make clean verify
