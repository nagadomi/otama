#!/bin/sh

if [ $# -ne 1 ]; then
    filename="./filelist.txt"
else
    filename=$1
fi
renice 19 -p $$
otama_create_database -c ./config.yaml
otama_import -c ./config.yaml $filename > id.txt
otama_pull -c ./config.yaml
rm -f ./data/ukbench.ldb
ruby make-database.rb

