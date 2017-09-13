#! /bin/bash
#
# $Id: oops.sh,v 1.1 1999/06/30 17:02:19 he Exp $
#
#  create a System.map suitable for ksymoops and alike that contains symbols
#  of resident kernel as well as of all currently loaded modules
#
KHOME=${KHOME:-/home/kernel}
KERNEL=${KERNEL:-linux-ix25}
cat $KHOME/$KERNEL/System.map > /tmp/System.map
for i in `cat /proc/modules | sed 's/ .*$//'`
do
	grep '^c' < /var/modules/$i.map | sed "s/$/@$i/"
done | sort >> /tmp/System.map

# write a script file which can be called in case of a kernel oops
# to decode the oops message (you need to compile ksymoops before using it).

cat > /tmp/oops_decode <<HERE
#!/bin/sh
cd /tmp
tail -50 /var/log/messages|sed 's/^.*kernel: //' |tee /dev/tty| $KHOME/$KERNEL/scripts/ksymoops /tmp/System.map
HERE
chmod a+x /tmp/oops_decode
