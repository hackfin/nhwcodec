Preface
============

This is an experimental branch as an attempt to restructure the code
for more understanding by non-insiders.
It is considered highly unstable. Do not use as reference. See Docker notes
below for testing against a working reference.

NHW Image Codec
============

A Next-Generation Free Open-Source Image Compression Codec

The NHW codec is an experimental codec that compresses for now 512x512 bitmap 24bit color images using notably a wavelet transform.

The NHW codec presents some innovations and a unique approach: more image neatness/sharpness, and aims to be competitive with current codecs like for example x265 (HEVC), Google WebP,...

Another advantage of the NHW codec is that it has a high speed, making it suitable for mobile, embedded devices.


How to compile?
============

For Windows: gcc *.c -O3 -o nhw_en/decoder.exe

For Linux: see Makefiles in sub folders

To encode an image (512x512 bitmap color image for now): nhw_encoder.exe imagename.png

encoder options: quality settings: -h1..3 or -l1..19

Windows:
=========
example: nhw_encoder.exe imagename.png -l3
                 
To decode: nhw_decoder.exe imagename.nhw

Linux:
=======

> nhwenc imagename.png -l3

> nhwdec imagename.nhw
>

Docker container:
=================

Docker containers have been proved to be an optimum way for testing
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
