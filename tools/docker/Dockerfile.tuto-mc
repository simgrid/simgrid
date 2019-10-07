# Base image 
FROM debian:testing

RUN apt update && apt -y upgrade 

RUN apt install -y sudo && \
    groupadd -g 999 user && \
    useradd -u 999 -g user user && \
    echo "user ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/user && \
    chmod 0440 /etc/sudoers.d/user && \
    mkdir -p /home/user && \
    chown -R user:user /home/user

# - Install SimGrid's dependencies 
# - Compile and install SimGrid itself.
# - Remove everything that was installed, and re-install what's needed by the SimGrid libraries before the Gran Final Cleanup
# - Keep g++ gcc gfortran as any MC user will use (some of) them
RUN apt install -y g++ gcc git valgrind gfortran libboost-dev libboost-all-dev cmake dpkg-dev libunwind-dev libdw-dev libelf-dev libevent-dev && \
    mkdir /source/ && cd /source && git clone --depth=1 https://framagit.org/simgrid/simgrid.git simgrid.git && \
    cd simgrid.git && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr/ -Denable_model-checking=ON -Denable_documentation=OFF -Denable_java=OFF -Denable_smpi=ON -Denable_compile_optimizations=ON . && \
    make -j4 install && \
    chown -R user:user /source && \
    mkdir debian/ && touch debian/control && dpkg-shlibdeps --ignore-missing-info lib/*.so -llib/ -O/tmp/deps && \
    apt remove -y git valgrind libboost-dev libboost-all-dev cmake dpkg-dev libunwind-dev libdw-dev libelf-dev libevent-dev && \
    apt install -y `sed -e 's/shlibs:Depends=//' -e 's/([^)]*)//g' -e 's/,//g' /tmp/deps` && rm /tmp/deps && \
    apt autoremove -y && apt autoclean && apt clean

USER user

# The build and dependencies are not cleaned in this image since it's it's highly experimental so far    
#    git reset --hard master && git clean -dfx && \

