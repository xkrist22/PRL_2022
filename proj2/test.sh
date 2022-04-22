#!/bin/bash

# setup
SEQUENCE=$1
SEQLEN=${#SEQUENCE}
PROCNUM=$(((2*SEQLEN)-1))
if (( PROCNUM < 0 )); then
	PROCNUM=1
fi

# compile
mpic++ --prefix /usr/local/share/OpenMPI -o  pro pro.cpp

# execute
mpirun -oversubscribe --prefix /usr/local/share/OpenMPI -np $PROCNUM pro $SEQUENCE
# teardown
rm -f pro

