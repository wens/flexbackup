#
# $Id$
# $Name$
#

CVSVER := $(shell echo v$(VER) | sed -e 's/\./_/g')
SITE := ../flexbackup/www
RPM  := /net/rpm
DATE := $(shell date)
CVS := cvs -d :ext:edwinh@cvs.sourceforge.net:/cvsroot/flexbackup

commit:
	$(CVS) commit

all: tar rpm lsm

tag: version commit

	$(CVS) tag -F $(CVSVER)

tar: version tag
	cd /tmp; $(CVS) export -r $(CVSVER) flexbackup; mv flexbackup flexbackup-$(VER)
	cd /tmp/flexbackup-$(VER); mv Makefile.dist Makefile
	cd /tmp/flexbackup-$(VER); mv flexbackup.spec.template flexbackup.spec
	cd /tmp/flexbackup-$(VER); perl -pi -e 's/%define version.*/%define version $(VER)/' flexbackup.spec
	tar -C /tmp -z -c -v -X tar.exclude -f $(SITE)/tarball/flexbackup-$(VER).tar.gz flexbackup-$(VER)
	rm -rf /tmp/flexbackup-$(VER)

rpm: version tar
	rpmbuild -ta $(SITE)/tarball/flexbackup-$(VER).tar.gz
	cp -p $(RPM)/RPMS/noarch/flexbackup-$(VER)-1.noarch.rpm $(SITE)/RPMS
	cp -p $(RPM)/SRPMS/flexbackup-$(VER)-1.src.rpm $(SITE)/RPMS
	rm $(RPM)/RPMS/noarch/flexbackup-$(VER)-1.noarch.rpm
	rm $(RPM)/SRPMS/flexbackup-$(VER)-1.src.rpm

version:
	test -n "$(VER)"
