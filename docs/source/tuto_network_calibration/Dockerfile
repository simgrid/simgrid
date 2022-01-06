FROM debian:11.1-slim

RUN printf '%s\n' \
    "deb    [check-valid-until=no, trusted=yes] http://snapshot.debian.org/archive/debian/20211101T024700Z/ bullseye main contrib non-free" \
    "deb-src [check-valid-until=no, trusted=yes] http://snapshot.debian.org/archive/debian/20211101T024700Z/ bullseye main contrib non-free" \
    "deb     [check-valid-until=no, trusted=yes] http://snapshot.debian.org/archive/debian/20211101T024700Z/ bullseye-updates main contrib non-free" \
    "deb-src [check-valid-until=no, trusted=yes] http://snapshot.debian.org/archive/debian/20211101T024700Z/ bullseye-updates main contrib non-free" > /etc/apt/sources.list \
&& \
    apt-get -o Acquire::Check-Valid-Until=false update \
&& \
    apt-get install -y --no-install-recommends \
# R
    r-base-core \
    r-cran-ggplot2 \
    r-cran-dplyr \
    r-cran-irkernel \
    r-cran-quantreg \
# simgrid dependencies
    g++ \
    gcc \
    git \
    valgrind \
    gfortran \
    libboost-dev \
    libboost-all-dev \
    cmake \
    dpkg-dev \
# misc tools
    curl \
# python
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-pandas \
    python3-matplotlib \
# jupyterlab dependencies
    libffi-dev \
    pandoc \
&& \
    apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# pycewise
RUN pip install --no-cache-dir --no-dependencies \
    pycewise==0.1.4 \
    plotnine==0.8.0 \
    patsy==0.5.2 \
    mizani==0.7.3 \
    palettable==3.3.0 \
    scipy==1.7.2 \
    statsmodels==0.13.0 \
    descartes==1.1.0

# R package not available at debian's cran
RUN R -e "install.packages('remotes', dependencies=TRUE, repos='http://cran.rstudio.com/')" && \
    R -e "require(remotes); install_version('Ckmeans.1d.dp','4.3.3', dependencies=TRUE, repos='http://cran.rstudio.com/')"

# python jupyterlab packages
RUN pip install --no-cache-dir --upgrade pip && \
    pip install --no-cache-dir \
    jupyterlab==3.2.3 \
    papermill==2.3.3 \
    ipywidgets==7.6.5

# simgrid
RUN mkdir -p /source && cd /source && git clone --depth=1 https://framagit.org/simgrid/simgrid.git simgrid.git && \
    cd simgrid.git && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr/ -Denable_documentation=OFF -Denable_smpi=ON -Denable_compile_optimizations=ON . && \
    make -j4 install

# platform-calibration
RUN cd /source && git clone --depth=1 https://framagit.org/simgrid/platform-calibration.git && \
    cd platform-calibration && \
    git checkout 899f044658246fb86f24e4efc96489df546ad3d3 && \
    cd src/calibration && \
    sed -i -e 's/#define NB_RUNS .*/#define NB_RUNS 1/g' calibrate.c && \
    make CC=smpicc

# get old calibration data for dahu cluster
RUN curl -s -L -o /source/simgrid.git/docs/source/tuto_network_calibration/grenoble_2018-08-29_1808878.zip \
    https://github.com/Ezibenroc/calibration_analysis/raw/4837a5d841380ab3dfdbfbc919eab485c08890e9/dahu/mpi/grenoble_2018-08-29_1808878.zip

# tuto_network_calibration compilation
RUN cd /source/simgrid.git/docs/source/tuto_network_calibration && \
    cmake . && \
    make

# prepare files for execution
RUN cd /source/simgrid.git/docs/source/tuto_network_calibration && \
    cp grenoble_2018-08-29_1808878.zip /tmp/ && \
    cd /tmp/ && unzip grenoble_2018-08-29_1808878.zip && \
    sed -i 1d exp.csv

WORKDIR /source/simgrid.git/docs/source/tuto_network_calibration/
ENTRYPOINT ["jupyter", "lab", "--ip=0.0.0.0", "--allow-root"]
