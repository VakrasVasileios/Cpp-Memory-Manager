#!/bin/bash

for s in 128 512 1024;
do
  for f in *.cpp
  do
    for t in "KB" "MB";
    do
      echo "g++ -std=c++17 -DCHUNK_SIZE_$t=$s -DHEAP_SIZE_$t=$s -DBENCHMARK=1 $f -o $s-$t-${f%.cpp}.out";
      g++ -std=c++17 -DCHUNK_SIZE_"$t"="$s" -DHEAP_SIZE_"$t"="$s" -DBENCHMARK=1 "$f" -o "$s-$t-${f%.cpp}.out";
      for i in $(seq 1 60);
      do
        echo "\tRun #$i of $s-$t-${f%.cpp}.out";
        "./$s-$t-${f%.cpp}.out" >> "$s-$t-${f%.cpp}.csv";
      done
      echo "rm ./$s-$t-${f%.cpp}.out";
      rm "./$s-$t-${f%.cpp}.out";
    done
  done
done

