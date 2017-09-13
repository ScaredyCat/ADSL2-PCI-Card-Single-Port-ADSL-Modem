#!/bin/sh
echo 8 > /proc/sys/kernel/printk

rmmod tcrypt


  #echo "md5 testing:"
insmod tcrypt.o mode=1
rmmod tcrypt
#echo "sha1 testing:"
insmod tcrypt.o mode=2
rmmod tcrypt

#rmmod des
#insmod des.o
#echo "DES ECB, CBC testing:"
insmod tcrypt.o mode=3
rmmod tcrypt
#echo "des3_ede testing:"
insmod tcrypt.o mode=4
rmmod tcrypt
#echo "AES testing:"
insmod tcrypt.o mode=10
rmmod tcrypt

