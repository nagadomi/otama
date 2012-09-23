#!/bin/sh
renice 19 -p $$
otama_create_table -c ./config.yaml
otama_import -c ./config.yaml ./filelist.txt > id.txt
otama_pull -c ./config.yaml
ruby make-database.rb

