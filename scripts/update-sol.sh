#!/usr/bin/env bash

[[ ! -d external/include ]] && mkdir -p external/include
cd external/include

[[ ! -d sol ]] && mkdir sol
cd sol

echo "Downloading Sol2..."

rm -rf ./sol*.hpp

wget https://raw.githubusercontent.com/ThePhD/sol2/develop/single/sol/sol.hpp > /dev/null 2>&1
wget https://raw.githubusercontent.com/ThePhD/sol2/develop/single/sol/sol_forward.hpp > /dev/null 2>&1
