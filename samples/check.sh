#!/bin/bash
for d in */ ; do
  echo "$d"
  diff <(ls $d|tr ' ' '_'|xargs basename -s.wav -a|sort) <(ls $d|tr ' ' '_'|sort|xargs basename -s.wav -a) >/dev/null
  if [ $? -ne 0 ]; then
    echo "$d has a problem.."
  fi
done

