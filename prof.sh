#!/bin/sh
$execute=timers
cmd="\"./$execute\" && gprof \"$execute\" gmon.out > \"prof_$execute.txt\" && rm gmon.out"
echo $cmd
eval $cmd
