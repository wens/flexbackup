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

all: tar rpm webdoc

tag: version commit
	perl -pi -e 's/^ README for version .*/ README for version $(VER)/' README
	perl -pi -e 's/^Version:        .*/Version:        $(VER)/' flexbackup.lsm.template
	perl -pi -e 's/^Entered-date:   .*/Entered-date:   $(DATE)/' flexbackup.lsm.template
	perl -pi -e 's/%define version .*/%define version $(VER)/' flexbackup.spec
	cvs commit -m "" flexbackup.lsm.template flexbackup.spec TODO README
	cvs tag -F $(CVSVER)

tar: tag
	cd /tmp; cvs export -r $(CVSVER) flexbackup; mv flexbackup flexbackup-$(VER)
	cd /tmp/flexbackup-$(VER); mv Makefile.dist Makefile
	tar -C /tmp -z -c -v -X tar.exclude -f $(SITE)/tarball/flexbackup-$(VER).tar.gz flexbackup-$(VER)
	cp -p $(SITE)/tarball/flexbackup-$(VER).tar.gz $(SITE)/tarball/flexbackup-latest.tar.gz
	rm -rf /tmp/flexbackup-$(VER)

rpm: tar
	rpmbuild -ta $(SITE)/tarball/flexbackup-$(VER).tar.gz
	cp -p $(RPM)/RPMS/noarch/flexbackup-$(VER)-1.noarch.rpm $(SITE)/RPMS
	cp -p $(RPM)/SRPMS/flexbackup-$(VER)-1.src.rpm $(SITE)/RPMS
	rm $(RPM)/RPMS/noarch/flexbackup-$(VER)-1.noarch.rpm
	rm $(RPM)/SRPMS/flexbackup-$(VER)-1.src.rpm

webdoc:
	cd /tmp; cvs export -r $(CVSVER) flexbackup
	cp /tmp/flexbackup/CHANGES /tmp/flexbackup/README /tmp/flexbackup/TODO $(SITE)
	rm -rf /tmp/flexbackup

version:
	test -n "$(VER)"
