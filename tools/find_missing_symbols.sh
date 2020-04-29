#!/bin/bash

f1="$1"
f2="$2"

f1funcs=$(grep '^[a-z]\+.*[)][;]*$' $f1 | sort)
f2funcs=$(grep '^[a-z]\+.*[)][;]*$' $f2 | sort)

f1symbols=$(echo "$f1funcs" | awk -F'(' '{ print $1 }')
f2symbols=$(echo "$f2funcs" | awk -F'(' '{ print $1 }')

echo "$f1symbols" > /tmp/f1
echo "$f2symbols" > /tmp/f2

diff=$(diff /tmp/f1 /tmp/f2);

if [ -z "$diff" ]; then
    # all match
    exit 0
fi

echo "> in $f2"
echo "< in $f1"
echo "-----"
echo "$diff"
