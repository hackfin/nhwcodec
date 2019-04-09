NHW Image Codec, experimental tiling tree
==========================================

Experimental Tiling branch, breaking legacy compatibility.
This aims at tiling support for the NHW codec to support somewhat arbitrary
image sizes.

A lot of work to be done:

- Fix overflow errors
- Enable residual coding for arbitrary tile resolutions (up to 512)
- Improve boundary conditions (reduce tiling artefacts)
- Make compression (huffman and tree coding) context aware (not per tile)

*Warning*: very unstable branch, possibly not working with many images and
showing artefacts.

Currently, tiling is only active when using the loopback test mode '-LT':

The '-s' parameter defines the tile power, usable values: 5..8

Usage example:

* Recode image with 32x32 tiles in LOW12 quality:

> ./nhwenc -LT -o /tmp/output.png -l12 -s5 /tmp/beach.png

* Recode image with 128x128 tiles in HIGH3 quality:

> ./nhwenc -LT -o /tmp/output.png -h3 -s7 /tmp/valley.png

How to compile?
===============

Compilation for Windows is currently unmaintained.
The libpng library package is required to compile.

For Linux: see Makefiles in sub folders

Docker container:
=================

Docker containers have been proven to be an optimum way for testing
the code and checking against reference implementations.

A Dockerfile is provided in docker/Dockerfile.

To build this container, you may use a local installation of the
Docker Toolbox on Windows or the corresponding package on a standard
Linux distribution.

Commands to build the container (inside docker/):

> docker build -t nhw .

Start the container:
> docker run -it nhw

Inside the container:

> cd; . init.sh

Now you should have all prerequisites installed.

1) Build Codec:

> cd ~/src/nhwcodec; make all

2) Run tests/batch conversion of test images in ~/work:

> cd ~/src/nhwcodec/test; make clean all
