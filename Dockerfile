FROM ubuntu:20.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update && apt-get install -y git build-essential cmake ninja-build && \
    apt-get install -y libpng-dev libglfw3-dev && \
    apt-get install -y libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev

RUN git clone --recursive https://github.com/brunoherbelin/vimix.git

RUN cd vimix && cmake -G Ninja && ninja
