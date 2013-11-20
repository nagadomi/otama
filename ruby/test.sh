#!/bin/sh

RUBY=ruby
if test $# -gt 0; then
    RUBY=$1
fi
$RUBY test.rb bovw2k.yaml && \
$RUBY test.rb bovw8k.yaml && \
$RUBY test.rb bovw512k.yaml && \
$RUBY test.rb bovw512k_iv.yaml && \
$RUBY test_similarity.rb bovw512k_nodb.yaml && \
$RUBY test_similarity.rb vlad_nodb.yaml
