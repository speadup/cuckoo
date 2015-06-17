#!/bin/sh
svn st | grep ? | awk -F\  '{print $2}' | grep -v '^download\|^temp' | xargs rm -rf
find \( -name '*.o' -o -name '*.rej' -o -name '*.orig' \) -print -delete

