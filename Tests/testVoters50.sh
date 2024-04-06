#!/bin/bash

command="./bin/mysort -i "Data/voters50.bin" -k 3 -e1 MergeSort -e2 MergeSort"

flag="$1"

space_command=""
valgrind_command="-v"

if [[ $flag == "$space_command" ]] # Simple Execution
then
   echo "$command"
   $command

elif [[ "$flag" == "$valgrind_command" ]] # Valgrind Execution
then
   echo "valgrind --leak-check=full $command"
   valgrind --leak-check=full $command
fi
