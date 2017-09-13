AUTOMAKE_OPTIONS=foreign
pkgdocdir=@VBOX_DOCDIR@
pkgdoc_DATA=vbox.sgml vbox.txt
DISTCLEANFILES=${pkgdoc_DATA}
MAINTAINERCLEANFILES=Makefile.in

%: %.in
	sed \
		-e "s#\@I4LVERSION\@#@VERSION@#g" \
        	-e "s#\@BINDIR\@#@CONFIG_BINDIR@#g" \
        	-e "s#\@SBINDIR\@#@CONFIG_SBINDIR@#g" \
        	-e "s#\@VERSION\@#@VERSION@#g" \
        	-e "s#\@VERDATE\@#@VERDATE@#g" \
        	-e "s#\@PACKAGE\@#@PACKAGE@#g" \
        	-e "s#\@SPOOLDIR\@#@VBOX_SPOOLDIR@#g" \
        	-e "s#\@I4LCONFDIR\@#@ISDN_GLOBAL_CONFIG@#g" \
        	-e "s#\@SYSCONFDIR\@#@ISDN_GLOBAL_CONFIG@#g" \
        	-e "s#\@LOGFILEDIR\@#@VBOX_LOGDIR@#g" \
        	-e "s#\@PIDFILEDIR\@#@VBOX_PIDDIR@#g" \
        	-e "s#\@LCKFILEDIR\@#@VBOX_LCKDIR@#g" \
		< $< > $@
