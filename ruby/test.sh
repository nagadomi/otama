#!/bin/sh

RUBY=ruby
SRCDIR=.
if test $# -gt 0; then
    RUBY=$1
fi
if test $# -gt 1; then
    SRCDIR=$2
fi
$RUBY $SRCDIR/test.rb $SRCDIR/bovw2k.yaml && \
$RUBY $SRCDIR/test.rb $SRCDIR/bovw8k.yaml && \
$RUBY $SRCDIR/test.rb $SRCDIR/bovw512k.yaml && \
$RUBY $SRCDIR/test.rb $SRCDIR/bovw512k_iv.yaml && \
$RUBY $SRCDIR/test_similarity.rb $SRCDIR/bovw512k_nodb.yaml && \
$RUBY $SRCDIR/test_similarity.rb $SRCDIR/vlad_nodb.yaml && \
$RUBY $SRCDIR/test_kvs.rb 
