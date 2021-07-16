FROM debian:10.10-slim

RUN echo " \
    deb    [check-valid-until=no, trusted=yes] http://snapshot.debian.org/archive/debian/20210707T150931Z/ buster main contrib non-free \n\
    deb-src [check-valid-until=no, trusted=yes] http://snapshot.debian.org/archive/debian/20210707T150931Z/ buster main contrib non-free \n\
    deb     [check-valid-until=no, trusted=yes] http://snapshot.debian.org/archive/debian-security/20210707T150931Z/ buster/updates main contrib non-free \n\
    deb-src [check-valid-until=no, trusted=yes] http://snapshot.debian.org/archive/debian-security/20210707T150931Z/ buster/updates main contrib non-free" > /etc/apt/sources.list 

RUN	apt-get -o Acquire::Check-Valid-Until=false update;

# emacs
RUN apt-get install -y \
    emacs \
    ess \
    elpa-htmlize

# R
RUN apt-get install -y \
    r-base-core \
    r-cran-ggplot2 \
    r-cran-dplyr \
    r-cran-plyr \
    r-cran-jsonlite \
    r-cran-gridextra

# simgrid dependencies
RUN apt install -y \
    g++ \
    gcc \
    git \
    valgrind \
    gfortran \
    libboost-dev \
    libboost-all-dev \
    cmake \
    dpkg-dev

RUN apt-get install -y \
    curl

RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# install ox-rst to convert org to rst
RUN mkdir /source && cd /source && \
    git clone https://github.com/msnoigrs/ox-rst.git ox-rst.git

# compile install simgrid
RUN cd /source && git clone --depth=1 https://framagit.org/simgrid/simgrid.git simgrid.git && \
    cd simgrid.git && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr/ -Denable_documentation=OFF -Denable_smpi=ON -Denable_compile_optimizations=ON . && \
    make -j4 install

## compile 
RUN cd /source/simgrid.git/docs/source/tuto_disk && \
    cmake . &&\
    make

# Generate HTML
RUN cd /source/simgrid.git/docs/source/tuto_disk && \
     emacs -l init.el -batch \
        --eval "(require 'package)" \
        --eval "(package-initialize)" \
        --eval "(setq enable-local-eval t)" \
        --eval "(setq enable-local-variables t)" \
        --eval "(setq ess-ask-for-ess-directory nil)" \
        --eval "(setq org-babel-execute-src-block t)" \
        --eval "(setq org-export-babel-evaluate t)" \
        analysis.org --funcall org-rst-export-to-rst

WORKDIR /source/simgrid.git/
ENTRYPOINT [ "/bin/bash"]
