#!/usr/bin/env bash

key_num=10000000

echo "all on nvm" >> log.csv
for (( thread_num = 1; thread_num <= 32 ; thread_num = thread_num + 1 )); do
    echo $thread_num
    echo -n $thread_num "," >> log.csv
    ./nvmkv $thread_num $key_num >> log.csv
    sleep 3
done

