# Base image 
FROM simgrid/stable

# - Clone simgrid-template-s4u, as it is needed by the tutorial
# - Add an empty makefile advising to run cmake before make, just in case
RUN apt update && apt install -y pajeng r-base r-cran-ggplot2 r-cran-dplyr cmake g++ git libboost-all-dev&& \
    cd /source && \
    git clone --depth=1 https://framagit.org/simgrid/simgrid-template-s4u.git simgrid-template-s4u.git && \
    printf "master-workers ping-pong:\n\t@echo \"Please run the following command before make:\";echo \"    cmake .\"; exit 1" > Makefile &&\
    apt autoremove -y && apt clean && apt autoclean