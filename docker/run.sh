#!/bin/bash
set -x

dirname=$(dirname "$0")
dirname=$(realpath "$dirname")
ws=$(dirname "$dirname")
echo $ws
home=${HOME}

#docker run -d --network=host -v $ws:$ws  -it livegraph:latest /bin/bash
docker run -d --network=host -v $home:$home -it  -e TZ=Asia/Shanghai livegraph:latest /bin/bash 
