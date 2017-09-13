AUTOMAKE_OPTIONS=foreign no-dependencies
bin_SCRIPTS=vboxmail vboxplay
EXTRA_DIST= vboxmail.in vboxplay.in
MAINTAINERCLEANFILES=Makefile.in 
DISTCLEANFILES=vboxmail vboxplay

%: %.in
	MANDATE=`grep CHECKIN $< | awk '{print $$4}'`; \
	sed \
		-e "s#\@MANDATE\@#$${MANDATE}#g" \
        	-e "s#\@BINDIR\@#@CONFIG_BINDIR@#g" \
        	-e "s#\@SBINDIR\@#@CONFIG_SBINDIR@#g" \
        	-e "s#\@VERSION\@#@VERSION@#g" \
        	-e "s#\@VERDATE\@#@VERDATE@#g" \
        	-e "s#\@PACKAGE\@#@PACKAGE@#g" \
        	-e "s#\@SPOOLDIR\@#@VBOX_SPOOLDIR@#g" \
        	-e "s#\@I4LCONFDIR\@#@ISDN_GLOBAL_CONFIG@#g" \
        	-e "s#\@LOGFILEDIR\@#@VBOX_LOGDIR@#g" \
        	-e "s#\@PIDFILEDIR\@#@VBOX_PIDDIR@#g" \
        	-e "s#\@LCKFILEDIR\@#@VBOX_LCKDIR@#g" \
        	-e "s#\@MAILCMD\@#/usr/bin/mail#g" \
		< $< > $@
