# This Makefile is not prepared in any way to be used stand-alone

EXECS = x25.echod tdu.echod wuauth_test

all: $(EXECS)

x25.echod: echod.o
	$(LINK.o) echod.o -o x25.echod

tdu.echod: tdud.o ../eft/libeft.a
	$(LINK.o) tdud.o -L../eft -left -o tdu.echod

tdud.o: ../eft/include/tdu_user.h ../eft/tdu.h

wuauth_test: wuauth_test.o ../wuauth/libwuauth.a
	$(LINK.o) wuauth_test.o -L../wuauth -lwuauth $(CRYPTLIB) -o wuauth_test

clean:
	rm -f $(EXECS) *.o 

.PHONY: all clean
