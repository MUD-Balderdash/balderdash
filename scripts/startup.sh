#!/usr/bin/bash


mud_dir=/home/mud/mud


#devel_pid=`ps x | grep "rom 9001" | grep -v "grep" | awk {'print $1'}`
mud_pid=`ps x | grep "rom 9000" | grep -v "grep" | awk {'print $1'}`

#if [ -z $devel_pid ]; then
#    cd ${mud_dir}/devel/area/
#    ./rom 9001 &
#fi

if [ -z $mud_pid ]; then
    cd ${mud_dir}/area/
    ./rom 9000 &
fi
    

