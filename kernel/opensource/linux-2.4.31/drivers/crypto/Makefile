# File: drivers/crypto/Makefile

O_TARGET := crypto.o

list-multi := ifxdeu.o padlock.o 

padlock-objs-$(CONFIG_CRYPTO_DEV_PADLOCK_AES) += padlock-aes.o
padlock-objs := padlock-generic.o $(padlock-objs-y)

obj-$(CONFIG_CRYPTO_DEV_PADLOCK) += padlock.o

ifxdeu-objs-$(CONFIG_CRYPTO_DEV_INCAIP1_DES) += ifxdeu-des.o
ifxdeu-objs-$(CONFIG_CRYPTO_DEV_INCAIP1_AES) += ifxdeu-aes.o
ifxdeu-objs-$(CONFIG_CRYPTO_DEV_INCAIP2_DES) += ifxdeu-des.o
ifxdeu-objs-$(CONFIG_CRYPTO_DEV_INCAIP2_AES) += ifxdeu-aes.o
ifxdeu-objs-$(CONFIG_CRYPTO_DEV_INCAIP2_SHA1) += ifxdeu-sha1.o
ifxdeu-objs-$(CONFIG_CRYPTO_DEV_INCAIP2_MD5) += ifxdeu-md5.o
ifxdeu-objs-$(CONFIG_CRYPTO_DEV_DANUBE_DES) += ifxdeu-des.o
ifxdeu-objs-$(CONFIG_CRYPTO_DEV_DANUBE_AES) += ifxdeu-aes.o
ifxdeu-objs-$(CONFIG_CRYPTO_DEV_DANUBE_SHA1) += ifxdeu-sha1.o
ifxdeu-objs-$(CONFIG_CRYPTO_DEV_DANUBE_MD5) += ifxdeu-md5.o
ifxdeu-objs-$(CONFIG_CRYPTO_DEV_DANUBE_DMA) += ifxdeu-dma.o

ifxdeu-objs := ifxdeu-generic.o $(ifxdeu-objs-y)

obj-$(CONFIG_CRYPTO_DEV_DEU) += ifxdeu.o



include $(TOPDIR)/Rules.make

ifxdeu.o: $(ifxdeu-objs)
	$(LD) -r  -o $@ $(ifxdeu-objs)
	$(STRIP) -g  $@ 
padlock.o: $(padlock-objs)
	$(LD) -r -o $@ $(padlock-objs)
	
	

	
