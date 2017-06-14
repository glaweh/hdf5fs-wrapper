#!/bin/bash
cd $1
find . -type f | sort | while read f ; do
    bytes=`du -b $f | awk '{print $1}'`;
    printf "%9d %s\n" "$bytes" "`md5sum $f`";
done
