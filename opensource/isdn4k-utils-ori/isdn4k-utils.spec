Summary: Bundled Utilities for configuring an using ISDN4Linux
Name: isdn4k-utils
Version: 3.0beta2
Release: 1
Copyright: GPL
Group: Base
Source0: ftp://ftp.franken.de/pub/isdn4linux/v2.0/isdn4k-utils-3.0beta2.tar.gz
URL: http://www.isdn4linux.de
#Distribution: Unknown
Vendor: The ISDN4Linux Team
Packager: Fritz Elfert <fritz@isdn4linux.de>
Requires: kernel >= 2.0.36
BuildRoot: /var/tmp/isdn4k-utils-root

%description
isdn4k-utils is a collection of various ISDN related utilities. This
package contains configuration tools for all ISDN adapters, supported
by Linux. Furthermore, several status-monitors are provided as well as
some ISDN-based applications. Namely ipppd, a PPP daemon for synchronous
PPP over ISDN; vbox, an answering-machine and (for use with AVM-B1 only)
capifax, a faxmachine.

%prep
# remove old directory
if [ "X" != "${RPM_BUILD_ROOT}X" ]; then
	rm -rf $RPM_BUILD_ROOT
fi
rm -rf $RPM_BUILD_DIR/isdn4k-utils-%{PACKAGE_VERSION}
mkdir $RPM_BUILD_DIR/isdn4k-utils-%{PACKAGE_VERSION}

# unpack main sources
%setup -n isdn4k-utils-%{PACKAGE_VERSION}

%build
cp .config.rpm .config
make RPM_OPT_FLAGS="$RPM_OPT_FLAGS" subconfig
make RPM_OPT_FLAGS="$RPM_OPT_FLAGS"

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/{sbin,bin,man,include}
mkdir -p $RPM_BUILD_ROOT/usr/lib/isdn
mkdir -p $RPM_BUILD_ROOT/etc/isdn
mkdir -p $RPM_BUILD_ROOT/dev
make DESTDIR=$RPM_BUILD_ROOT install
cd $RPM_BUILD_ROOT/etc/isdn
for i in *.new ; do
	mv $i `basename $i .new`
done

%clean
if [ "X" != "${RPM_BUILD_ROOT}X" ]; then
	rm -rf $RPM_BUILD_ROOT
fi

%files
/dev
/sbin
/usr/bin
/usr/lib/isdn
/usr/lib/libcapi20.a
/usr/include
/usr/X11R6/include/X11/bitmaps
%config /usr/X11R6/lib/X11/app-defaults/*
%dir /etc/isdn
%config /etc/isdn/vboxgetty.conf
%config /etc/isdn/vboxd.conf
%config /etc/isdn/isdn.conf
%doc /usr/man/man1/*
%doc /usr/man/man4/*
%doc /usr/man/man5/*
%doc /usr/man/man7/*
%doc /usr/man/man8/*
%doc /usr/doc/vbox/*
%doc /usr/X11R6/man/man1/*
%doc /usr/doc/faq/isdn4linux/*
%dir /var/spool/vbox
%dir /var/log/vbox
