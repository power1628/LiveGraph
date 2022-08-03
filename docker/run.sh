#!/bin/bash
set -x

dirname=$(dirname "$0")
dirname=$(realpath "$dirname")
ws=$(dirname "$dirname")
echo $ws

docker run -d --network=host -v $ws:$ws  -it livegraph:latest /bin/bash
