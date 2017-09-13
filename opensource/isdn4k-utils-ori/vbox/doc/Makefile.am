AUTOMAKE_OPTIONS=foreign no-dependencies
SUBDIRS=de
man_MANS=autovbox.1 rmdtovbox.1 vbox_file.5 vbox.conf.5 vboxbeep.1 vboxconvert.1 vboxd.8 vboxd.conf.5 vboxgetty.8 vboxgetty.conf.5 vboxmode.1 vboxrc.5 vboxtcl.5 vboxtoau.1 vboxctrl.1 vboxmail.8 vboxplay.1 vboxputty.8 vbox.1
DISTCLEANFILES=${man_MANS}
EXTRA_DIST=autovbox.man rmdtovbox.man vbox.conf.man vbox_file.man vboxbeep.man vboxconvert.man vboxd.conf.man vboxd.man vboxgetty.conf.man vboxgetty.man vboxmode.man vboxrc.man vboxtcl.man vboxtoau.man vboxctrl.man vboxmail.man vboxplay.man vboxputty.man vbox.man

%.1 %.5 %.8: %.man
	MANDATE=`grep CHECKIN $< | awk '{print $$4}'`; \
	sed \
		-e "s#\@MANDATE\@#$${MANDATE}#g" \
		-e "s#\@I4LVERSION\@#$${I4LVERSION}#g" \
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
		< $< > $@
