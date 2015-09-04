#!/bin/bash

#Use: chmod 755 ./script.sh. Parameters: First 3 are the ep1 parameters, last one is the total number of times the program will be executed.

number=$1
input_name=$2
output_name=$3
runs=$4

count=1
while [ $count -le $runs ]; do
  echo $'\nRun #'$count$'\n'
  ./ep1 $number $input_name $output_name
  total_process=$(wc -l < outputs/$output_name)
  total_process=$(($total_process - 1))

  i=0
  while read p; do
    echo $i and ${p}
    process_finished[$i]=$(echo ${p} | cut -d ' ' -f 2)
    process_duration[$i]=$(echo ${p} | cut -d ' ' -f 3)
    echo aqui ${process_finished[$i]} ${process_duration[$i]}
    if [ $count -eq 1 ]; then
      process_finished_all_runs[$i]=${process_finished[$i]}
      process_duration_all_runs[$i]=${process_duration[$i]}
    else
      process_finished_all_runs[$i]=$(echo "${process_finished_all_runs[$i]}+${process_finished[$i]}" | bc -l )
      process_duration_all_runs[$i]=$(echo "${process_duration_all_runs[$i]}+${process_duration[$i]}" | bc -l )
    fi
    echo ${process_finished_all_runs[$i]}
    i=$(($i + 1))
    if [ $i -ge $total_process ]; then
      break
    fi
  done < outputs/$output_name
  let count=count+1
done

i=0
while [ $i -lt $total_process ]; do
  echo '\n'process $(($i + 1)):
  process_finished_all_runs[$i]=$(echo "${process_finished_all_runs[$i]}/$runs" | bc -l )
  process_duration_all_runs[$i]=$(echo "${process_duration_all_runs[$i]}/$runs" | bc -l )
  echo finished mean: ${process_finished_all_runs[$i]}  duration_mean: ${process_duration_all_runs[$i]}
  let i=i+1
done
