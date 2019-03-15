FROM debian:latest

RUN apt-get update; apt-get install -y \
	make git gcc libpng-dev vim wget unzip

RUN useradd -u 1000 -g 100 -m -s /bin/bash nhw 
RUN mkdir /home/nhw/src
RUN mkdir /home/nhw/work

COPY init.sh /home/nhw

RUN chown -R nhw /home/nhw


# USER nhw
