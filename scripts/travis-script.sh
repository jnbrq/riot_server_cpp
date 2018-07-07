#!/usr/bin/env bash

cd build
make
ctest --output-on-failure
