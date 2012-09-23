#!/bin/sh

ruby test.rb bovw2k.yaml && \
ruby test.rb bovw8k.yaml && \
ruby test.rb bovw512k.yaml && \
ruby test.rb bovw512k_iv.yaml && \
ruby test_similarity.rb bovw512k_nodb.yaml && \
ruby test_similarity.rb vlad_nodb.yaml
