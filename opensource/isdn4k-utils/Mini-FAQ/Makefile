all:    isdn-faq.html

isdn-faq.html:    isdn-faq.txt htmlify.pl Makefile
	./htmlify.pl isdn-faq.txt > isdn-faq.html

clean:
	rm -f isdn-faq.html

distclean: clean
