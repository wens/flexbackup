#
# $Id$
# $Name$
#

CVSVER := $(shell echo v$(VER) | sed -e 's/\./_/g')
SITE := ./www
RPM  := /net/rpm
DATE := $(shell date)
CVSROOT := :ext:edwinh@cvs.sourceforge.net:/cvsroot/flexbackup

commit:
	cvs commit

all: tar rpm lsm

tag: version commit

	cvs tag -F $(CVSVER)

lsm: version
	cp flexbackup.lsm.template flexbackup.lsm
	perl -pi -e 's/VERSION/$(VER)/' flexbackup.lsm
	perl -pi -e 's/DATE/$(DATE)/' flexbackup.lsm

tar: version tag
	cd /tmp; cvs export -r $(CVSVER) flexbackup; mv flexbackup flexbackup-$(VER)
	cd /tmp/flexbackup-$(VER); mv Makefile.dist Makefile
	cd /tmp/flexbackup-$(VER); mv flexbackup.spec.template flexbackup.spec
	cd /tmp/flexbackup-$(VER); perl -pi -e 's/%define version.*/%define version $(VER)/' flexbackup.spec
	tar -C /tmp -z -c -v -X tar.exclude -f $(SITE)/tarball/flexbackup-$(VER).tar.gz flexbackup-$(VER)
	ln -snf flexbackup-$(VER).tar.gz $(SITE)/tarball/flexbackup-latest.tar.gz
	cp /tmp/flexbackup-$(VER)/CHANGES /tmp/flexbackup-$(VER)/README /tmp/flexbackup-$(VER)/TODO $(SITE)
	rm -rf /tmp/flexbackup-$(VER)

rpm: version tar
	rpmbuild -ta $(SITE)/tarball/flexbackup-$(VER).tar.gz
	cp -p $(RPM)/RPMS/noarch/flexbackup-$(VER)-1.noarch.rpm $(SITE)/RPMS
	cp -p $(RPM)/SRPMS/flexbackup-$(VER)-1.src.rpm $(SITE)/RPMS
	rm $(RPM)/RPMS/noarch/flexbackup-$(VER)-1.noarch.rpm
	rm $(RPM)/SRPMS/flexbackup-$(VER)-1.src.rpm

rsync:
	rsync --archive --verbose --delete $(SITE)/ edwinh.org:public_html/flexbackup
	rsync --archive --verbose --delete $(SITE)/ flexbackup.sourceforge.net:/home/groups/f/fl/flexbackup/htdocs

upload: version lsm
	test -e www/tarball/flexbackup-$(VER).tar.gz
	ncftpput ibiblio.org /incoming/Linux www/tarball/flexbackup-$(VER).tar.gz
	ncftpput ibiblio.org /incoming/Linux flexbackup.lsm
	ncftpput incoming.redhat.com /noarch www/RPMS/flexbackup-$(VER)-1.noarch.rpm
	ncftpput incoming.redhat.com /noarch www/RPMS/flexbackup-$(VER)-1.src.rpm

version:
	test -n "$(VER)"
