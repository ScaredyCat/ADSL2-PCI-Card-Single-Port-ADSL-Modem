all: hostap-config

clean:
	rm -rf *.o
	rm -rf hostap-config
	rm -rf hostapd.conf


hostap-config: hostap-config.c
	gcc -o $@ $^	
