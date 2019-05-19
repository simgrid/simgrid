# Base image 
FROM debian:testing

# - Install SimGrid's dependencies
RUN apt update && \
    apt install -y \
       g++ gcc gfortran default-jdk pybind11-dev \
       git \
       valgrind \
       libboost-dev libboost-all-dev \
       cmake \
       python3-pip \
       doxygen fig2dev \
       chrpath \
       libdw-dev libevent-dev libunwind8-dev \
       linkchecker \
       && \
    pip3 install breathe javasphinx 'sphinx>=1.8.0b1' sphinx_rtd_theme
		   