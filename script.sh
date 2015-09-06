#!/bin/bash

#Use: chmod 755 ./script.sh. Parameters: First 3 are the ep1 parameters, last one is the total number of times the program will be executed.

declare -A process_finished
declare -A process_duration
number=$1
input_name=$2
output_name=$3
runs=$4

# execute the ep1 'runs' times

count=1
while [ $count -le $runs ]; do
  run=$(($count - 1))
  echo Bash Script Run $'#'$count:
  ./ep1 $number $input_name $output_name
  total_process=$(wc -l < outputs/$output_name)
  total_process=$(($total_process - 1))
  i=0
  while read p; do
    process_finished[$i,$run]=$(echo ${p} | cut -d ' ' -f 2)
    process_duration[$i,$run]=$(echo ${p} | cut -d ' ' -f 3)
    if [ $count -eq 1 ]; then
      process_finished_all_runs[$i]=${process_finished[$i,$run]}
      process_duration_all_runs[$i]=${process_duration[$i,$run]}
    else
      process_finished_all_runs[$i]=$(echo "${process_finished_all_runs[$i]}+${process_finished[$i,$run]}" | bc -l )
      process_duration_all_runs[$i]=$(echo "${process_duration_all_runs[$i]}+${process_duration[$i,$run]}" | bc -l )
    fi
    i=$(($i + 1))
    if [ $i -ge $total_process ]; then
      break
    fi
  done < outputs/$output_name
  let count=count+1
done

#get the mean of both output time values (ending time and duration)
i=0
while [ $i -lt $total_process ]; do
  process_finished_mean[$i]=$(echo "${process_finished_all_runs[$i]}/$runs" | bc -l )
  process_duration_mean[$i]=$(echo "${process_duration_all_runs[$i]}/$runs" | bc -l )
  let i=i+1
done

#get the standard deviation
i=0
while [ $i -lt $total_process ]; do
  process_finished_sd[$i]=0
  process_duration_sd[$i]=0
  j=0
  while [ $j -lt $runs ]; do
    fterm[$i]=$(echo "${process_finished[$i,$j]}-${process_finished_mean[$i]}" | bc -l )
    fterm[$i]=$(echo "${fterm[$i]}*${fterm[$i]}" | bc -l )
    process_finished_sd[$i]=$(echo "${process_finished_sd[$i]}+${fterm[$i]}" | bc -l )
    dterm[$i]=$(echo "${process_duration[$i,$j]}-${process_duration_mean[$i]}" | bc -l )
    dterm[$i]=$(echo "${dterm[$i]}*${dterm[$i]}" | bc -l )
    process_duration_sd[$i]=$(echo "${process_duration_sd[$i]}+${dterm[$i]}" | bc -l )
    let j=j+1
  done
  # Variances of process i
  process_finished_sd[$i]=$(echo "${process_finished_sd[$i]}/$runs" | bc -l )
  process_duration_sd[$i]=$(echo "${process_duration_sd[$i]}/$runs" | bc -l )
  # SDs of process i
  process_finished_sd[$i]=$(echo "sqrt(${process_finished_sd[$i]})" | bc -l )
  process_duration_sd[$i]=$(echo "sqrt(${process_duration_sd[$i]})" | bc -l )
  let i=i+1
done

#get the confidence interval of 95%
i=0
while [ $i -lt $total_process ]; do
  # lower endpoint
  process_finished_lower_endpoint[$i]=$(echo "-1.96*${process_finished_sd[$i]}" | bc -l )
  process_finished_lower_endpoint[$i]=$(echo "${process_finished_lower_endpoint[$i]}/sqrt($total_process)" | bc -l )
  process_finished_lower_endpoint[$i]=$(echo "${process_finished_lower_endpoint[$i]}+${process_finished_mean[$i]}" | bc -l )
  process_duration_lower_endpoint[$i]=$(echo "-1.96*${process_duration_sd[$i]}" | bc -l )
  process_duration_lower_endpoint[$i]=$(echo "${process_duration_lower_endpoint[$i]}/sqrt($total_process)" | bc -l )
  process_duration_lower_endpoint[$i]=$(echo "${process_duration_lower_endpoint[$i]}+${process_duration_mean[$i]}" | bc -l )
  # upper endpoint
  process_finished_upper_endpoint[$i]=$(echo "1.96*${process_finished_sd[$i]}" | bc -l )
  process_finished_upper_endpoint[$i]=$(echo "${process_finished_upper_endpoint[$i]}/sqrt($total_process)" | bc -l )
  process_finished_upper_endpoint[$i]=$(echo "${process_finished_upper_endpoint[$i]}+${process_finished_mean[$i]}" | bc -l )
  process_duration_upper_endpoint[$i]=$(echo "1.96*${process_duration_sd[$i]}" | bc -l )
  process_duration_upper_endpoint[$i]=$(echo "${process_duration_upper_endpoint[$i]}/sqrt($total_process)" | bc -l )
  process_duration_upper_endpoint[$i]=$(echo "${process_duration_upper_endpoint[$i]}+${process_duration_mean[$i]}" | bc -l )
  let i=i+1
done

# prints mean | SD | CI of each process i
i=0
echo $'\n'process i MEAN '/' SD '/' CI. "(F) stands for finished and (D) for duration":
while [ $i -lt $total_process ]; do
  echo "(F) process"$(($i + 1)) ${process_finished_mean[$i]} '/' ${process_finished_sd[$i]} '/' '['${process_finished_lower_endpoint[$i]}, ${process_finished_upper_endpoint[$i]}']'
  echo "(D) process"$(($i + 1)) ${process_duration_mean[$i]} '/' ${process_duration_sd[$i]} '/' '['${process_duration_lower_endpoint[$i]}, ${process_duration_upper_endpoint[$i]}']'
  let i=i+1
done
