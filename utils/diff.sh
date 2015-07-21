#!/bin/sh
opt="-Nur -x *.orig -x *.rej -x *.o -x *.log -x tags -x swp"
diff ${opt} $@ | sed 's@\t[0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\} [0-9]\{2\}:[0-9]\{2\}:[0-9]\{2\}.*+[0-9]\{4\}$@@'
