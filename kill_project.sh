#!/bin/bash

pids=`ps  -u | grep "./RaceSimulator" | grep -v "grep" | awk 'NR>0{print$2}'`

for pid in $pids; do
  kill $pid
done

rm -f manager
./kill_ipcs.sh
