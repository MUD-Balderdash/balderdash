#!/bin/bash

curl http://proolnewtest.kharkov.org/?balderdash=`cat /home/mud/mud/area/wwwcount.txt |  grep -a "64" | awk -F: {'print $2'} | awk -F. {'print $1'} | awk {'print $1'} | awk {'print $1'}`
