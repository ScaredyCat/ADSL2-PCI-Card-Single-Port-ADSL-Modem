data_DATA = isdnlog.xpm

datadir = $(prefix)/share/apps/@PACKAGE@/pics

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(appsdir)
	$(INSTALL_DATA) @PACKAGE@.xpm $(DESTDIR)$(kde_icondir)/@PACKAGE@.xpm
	$(INSTALL_DATA) isdnlog.xpm $(DESTDIR)$(kde_datadir)/@PACKAGE@/isdnlog.xpm

uninstall-local:
	rm -f $(DESTDIR)$(kde_icondir)/@PACKAGE@.xpm
	rm -f $(DESTDIR)$(kde_datadir)/@PACKAGE@/isdnlog.xpm
