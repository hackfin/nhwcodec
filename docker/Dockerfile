FROM debian:latest

RUN apt-get update; apt-get install -y \
	make git gcc libpng-dev vim wget unzip python bsdmainutils \
	valgrind

RUN useradd -u 1000 -g 100 -m -s /bin/bash nhw 
RUN mkdir /home/nhw/src
RUN mkdir /home/nhw/work

COPY init.sh /home/nhw
COPY build_test.sh /home/nhw
COPY build_check.sh /home/nhw

RUN chown -R nhw /home/nhw

USER nhw
WORKDIR /home/nhw
RUN bash /home/nhw/init.sh
