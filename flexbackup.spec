Summary: Flexible backup script
Name: flexbackup
Version: 0.7
Release: 1
Source: http://members.home.net/edwinh/flexbackup/flexbackup-0.7.tar.gz
Url: http://members.home.net/edwinh/flexbackup/
Copyright: GPL
Group: Applications/System
Buildroot: /var/tmp/flexbackup-root
Requires: perl
Requires: mt-st

%description
Flexible backup script.
Features:
   o Easy to configure (at least I think so)
   o Uses dump, afio, tar, or cpio with the flick of a switch
   o Backup, extract, compare, list modes
   o Full (0) and 1-9 levels of incremental backup
   o Filesystem-oriented (wont traverse devices)
   o Does remote filesystems (over rsh/ssh; no special service)
   o Works with IDE/SCSI tapes on Linux/FreeBSD
     or Linux ftape (allows table of contents support)
   o Nice log files

%prep
%setup -q

%build

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc
mkdir -p $RPM_BUILD_ROOT/bin
install -m 755 flexbackup.conf $RPM_BUILD_ROOT/etc
install -m 755 flexbackup $RPM_BUILD_ROOT/bin

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc CHANGES COPYING TODO README flexbackup.lsm
%dir /var/lib/flexbackup
%dir /var/log/flexbackup
%config /etc/flexbackup.conf

%changelog
* Sat Sep 18 18:14:15 MST 1999 Edwin Huffstutler <edwinh@computer.org>
- initial rpm package
