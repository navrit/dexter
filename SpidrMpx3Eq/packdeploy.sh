#!/bin/bash

if [ ${#@} -lt 1 ] ; then
    echo "use $0 deploymentDir(string)"
    exit
fi

depdir=$1

mkdir $depdir

cp -Rf ./config $depdir
cp -Rf ./shaders $depdir
cp -Rf ./assemblies $depdir
cp -Rf ./data $depdir
cp -Rf ./icons $depdir

echo "[DONE]"
