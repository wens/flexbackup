#
# $Id$
# $Name$
#

CVSVER := $(shell echo v$(VER) | sed -e 's/\./_/g')
SITE := ./www
RPM  := /usr/src/redhat
DATE := $(shell date)

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

tar: tag lsm
	cd /tmp; cvs co -r $(CVSVER) flexbackup; mv flexbackup flexbackup-$(VER)
	cd /tmp/flexbackup-$(VER); mv Makefile.dist Makefile
	tar -C /tmp -z -c -v -X tar.exclude -f $(SITE)/tarball/flexbackup-$(VER).tar.gz flexbackup-$(VER)
	cp -p $(SITE)/tarball/flexbackup-$(VER).tar.gz $(SITE)/tarball/flexbackup-latest.tar.gz
	cd /tmp; echo yes | cvs release -d flexbackup-$(VER)

rpm: tar
	sudo cp $(SITE)/tarball/flexbackup-$(VER).tar.gz $(RPM)/SOURCES
	sudo rpm -ba flexbackup.spec
	cp -p $(RPM)/RPMS/noarch/flexbackup-$(VER)-1.noarch.rpm $(SITE)/RPMS
	cp -p $(RPM)/SRPMS/flexbackup-$(VER)-1.src.rpm $(SITE)/RPMS
	sudo rpm --rmsource flexbackup.spec
	sudo rm $(RPM)/RPMS/noarch/flexbackup-$(VER)-1.noarch.rpm
	sudo rm $(RPM)/SRPMS/flexbackup-$(VER)-1.src.rpm

webdoc:
	cd /tmp; cvs co -r $(CVSVER) flexbackup
	cp /tmp/flexbackup/CHANGES /tmp/flexbackup/README /tmp/flexbackup/TODO $(SITE)
	cd /tmp; echo yes | cvs release -d flexbackup

lsm: tag
	cd /tmp; cvs co -r $(CVSVER) flexbackup; mv flexbackup flexbackup-$(VER)
	tar -C /tmp -z -c -v -X tar.exclude -f $(SITE)/tarball/flexbackup-$(VER).tar.gz flexbackup-$(VER)
	cd /tmp; echo yes | cvs release -d flexbackup-$(VER)
	./sizelsm.perl $(SITE)/tarball $(VER)
	cvs commit -m "" flexbackup.lsm
	cvs tag -F $(CVSVER) flexbackup.lsm

version:
	test -n "$(VER)"
