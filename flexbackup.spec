Summary: Flexible backup script
Name: flexbackup
%define version 0.9.8.1
Version: %{version}
Release: 1
Packager: Edwin Huffstutler <flexbackup@home.com>
Source: http://members.home.com/flexbackup/tarball/flexbackup-%{version}.tar.gz
Url: http://flexbackup.sourceforge.net
Copyright: GPL
Group: Applications/Archiving
BuildRoot: /var/tmp/flexbackup-root
BuildArch: noarch
Requires: mt-st
Requires: gzip
Requires: bzip2
Requires: ncompress
Requires: fileutils
Requires: findutils

%description
Flexible backup script.

Features:
   o Easy to configure
   o Uses dump, afio, tar, or cpio with the flick of a switch
   o Backup, extract, compare, list modes
   o Compression and buffering for all backup types
   o Full (0) and 1-9 levels of incremental backup
   o Keeps a table of contents so you know whats on each tape
   o Does remote filesystems (over rsh/ssh; no special service)
   o Works with IDE/SCSI tapes on Linux/FreeBSD, Linux ftape, or disk files
   o Nice log files


%prep
%setup -q

%build
make all

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/var/state/flexbackup
mkdir -p $RPM_BUILD_ROOT/var/log/flexbackup
install -m 0644 flexbackup.conf $RPM_BUILD_ROOT/etc/flexbackup.conf
install -m 0755 fb.install $RPM_BUILD_ROOT/usr/bin/flexbackup
install -m 0755 multibuf $RPM_BUILD_ROOT/usr/bin/multibuf


%clean
rm -rf $RPM_BUILD_ROOT

%files
/usr/bin/flexbackup
/usr/bin/multibuf
%defattr(-,root,root)
%doc CHANGES COPYING TODO README CREDITS flexbackup.lsm INSTALL
%dir /var/state/flexbackup
%dir /var/log/flexbackup
%config /etc/flexbackup.conf

%changelog
* Sun Nov  7 1999 Edwin Huffstutler <edwinh@computer.org>
- add multibuf

* Sat Sep 25 1999 Edwin Huffstutler <edwinh@computer.org>
- add more requires, update description, email address.
- really goes in /usr/bin since it needs perl anyway --
  if you only have your root fs, run restore or tar by hand :)

* Sat Sep 18 1999 Edwin Huffstutler <edwinh@computer.org>
- initial rpm package
