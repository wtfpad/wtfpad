#!/bin/bash

# This code scripts and parametrizes the k-NN attack.

ABS_PATH=`cd "$1"; pwd`
SITES=100
INST_DIST_LEARN=22
INST_TEST=11
TOTAL_INST=33

pushd `dirname $0` > /dev/null

# reset batch
mkdir -p batch
rm -rf batch/*

# extract features
python fextractor.py -o batch -c $SITES -i $TOTAL_INST -p $ABS_PATH

# compile attack
g++ flearner.cpp -o flearner

# run attack
./flearner $SITES $INST_DIST_LEARN $INST_TEST

# print accuracy
echo "Accuracy (plus/minus 1% variance):"
cat accuracy
popd > /dev/null
