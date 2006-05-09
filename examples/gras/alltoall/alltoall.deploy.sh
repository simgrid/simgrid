#!/bin/sh
############ DEPLOYMENT FILE #########
if test "${MACHINES+set}" != set; then 
    export MACHINES='Bourassa Fafard Ginette Jupiter Tremblay ';
fi
if test "${INSTALL_PATH+set}" != set; then 
    export INSTALL_PATH='`echo $HOME`/tmp/src'
fi
if test "${GRAS_ROOT+set}" != set; then 
    export GRAS_ROOT='`echo $INSTALL_PATH`'
fi
if test "${SRCDIR+set}" != set; then 
    export SRCDIR=./
fi
if test "${SIMGRID_URL+set}" != set; then 
    export SIMGRID_URL=http://gcl.ucsd.edu/simgrid/dl/
fi
if test "${SIMGRID_VERSION+set}" != set; then 
    export SIMGRID_VERSION=2.91
fi
if test "${GRAS_PROJECT+set}" != set; then 
    export GRAS_PROJECT=alltoall
fi
if test "${GRAS_PROJECT_URL+set}" != set; then 
    export GRAS_PROJECT_URL=http://www-id.imag.fr/Laboratoire/Membres/Legrand_Arnaud/gras_test/
fi

test -e runlogs/ || mkdir -p runlogs/
cmd_prolog="env INSTALL_PATH=$INSTALL_PATH GRAS_ROOT=$GRAS_ROOT \
                 SIMGRID_URL=$SIMGRID_URL SIMGRID_VERSION=$SIMGRID_VERSION GRAS_PROJECT=$GRAS_PROJECT \
                 GRAS_PROJECT_URL=$GRAS_PROJECT_URL LD_LIBRARY_PATH=$GRAS_ROOT/lib/ sh -c ";
cmd="\$INSTALL_PATH/gras-alltoall/alltoall_node 4000 Jupiter Fafard Ginette Bourassa ";
ssh Tremblay "$cmd_prolog 'export LD_LIBRARY_PATH=\$INSTALL_PATH/lib:\$LD_LIBRARY_PATH; echo \"$cmd\" ; $cmd 2>&1'" > runlogs/Tremblay_0.log &
cmd="\$INSTALL_PATH/gras-alltoall/alltoall_node 4000 Tremblay Fafard Ginette Bourassa ";
ssh Jupiter "$cmd_prolog 'export LD_LIBRARY_PATH=\$INSTALL_PATH/lib:\$LD_LIBRARY_PATH; echo \"$cmd\" ; $cmd 2>&1'" > runlogs/Jupiter_1.log &
cmd="\$INSTALL_PATH/gras-alltoall/alltoall_node 4000 Tremblay Jupiter Ginette Bourassa ";
ssh Fafard "$cmd_prolog 'export LD_LIBRARY_PATH=\$INSTALL_PATH/lib:\$LD_LIBRARY_PATH; echo \"$cmd\" ; $cmd 2>&1'" > runlogs/Fafard_2.log &
cmd="\$INSTALL_PATH/gras-alltoall/alltoall_node 4000 Tremblay Jupiter Fafard Bourassa ";
ssh Ginette "$cmd_prolog 'export LD_LIBRARY_PATH=\$INSTALL_PATH/lib:\$LD_LIBRARY_PATH; echo \"$cmd\" ; $cmd 2>&1'" > runlogs/Ginette_3.log &
cmd="\$INSTALL_PATH/gras-alltoall/alltoall_node 4000 Tremblay Jupiter Fafard Ginette ";
ssh Bourassa "$cmd_prolog 'export LD_LIBRARY_PATH=\$INSTALL_PATH/lib:\$LD_LIBRARY_PATH; echo \"$cmd\" ; $cmd 2>&1'" > runlogs/Bourassa_4.log &
