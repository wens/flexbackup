Summary: Flexible backup script
Name: flexbackup
%define version 0.9.5.4
Version: %{version}
Release: 1
Packager: Edwin Huffstutler <edwinh+flexbackup@computer.org>
Source: http://members.home.com/edwinh/flexbackup/flexbackup-%{version}.tar.gz
Url: http://members.home.com/edwinh/flexbackup/
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
   o Filesystem-oriented (will not traverse devices)
   o Does remote filesystems (over rsh/ssh; no special service)
   o Works with IDE/SCSI tapes on Linux/FreeBSD, Linux ftape, or disk files
   o Nice log files


%prep
%setup -q

%build

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/var/lib/flexbackup
mkdir -p $RPM_BUILD_ROOT/var/log/flexbackup
make fb.install
install -m 0644 flexbackup.conf $RPM_BUILD_ROOT/etc/flexbackup.conf
install -m 0755 fb.install $RPM_BUILD_ROOT/usr/bin/flexbackup


%clean
rm -rf $RPM_BUILD_ROOT

%files
/usr/bin/flexbackup
%defattr(-,root,root)
%doc CHANGES COPYING TODO README CREDITS flexbackup.lsm INSTALL
%dir /var/lib/flexbackup
%dir /var/log/flexbackup
%config /etc/flexbackup.conf

%changelog
* Sat Sep 25 1999 Edwin Huffstutler <edwinh@computer.org>
- add more requires, update description, email address.
- really goes in /usr/bin since it needs perl anyway --
  if you only have your root fs, run restore or tar by hand :)

* Sat Sep 18 1999 Edwin Huffstutler <edwinh@computer.org>
- initial rpm package
