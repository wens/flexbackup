Summary: Flexible backup script
Name: flexbackup
%define version 0.9.5a2
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
Requires: buffer
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
   o Filesystem-oriented (won't traverse devices)
   o Does remote filesystems (over rsh/ssh; no special service)
   o Works with IDE/SCSI tapes on Linux/FreeBSD, Linux ftape, or disk files
   o Nice log files


%prep
%setup -q

%build

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc
mkdir -p $RPM_BUILD_ROOT/bin
mkdir -p $RPM_BUILD_ROOT/var/lib/flexbackup
mkdir -p $RPM_BUILD_ROOT/var/log/flexbackup
install -m 755 flexbackup.conf $RPM_BUILD_ROOT/etc
install -m 755 flexbackup $RPM_BUILD_ROOT/bin

%clean
rm -rf $RPM_BUILD_ROOT

%files
/bin/flexbackup
%defattr(-,root,root)
%doc CHANGES COPYING TODO README flexbackup.lsm
%dir /var/lib/flexbackup
%dir /var/log/flexbackup
%config /etc/flexbackup.conf

%changelog
* Sat Sep 25 1999 Edwin Huffstutler <edwinh@computer.org>
- add more requires, update description, email address

* Sat Sep 18 1999 Edwin Huffstutler <edwinh@computer.org>
- initial rpm package
