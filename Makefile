
CVSVER := $(shell echo v$(VER) | sed -e 's/\./_/g')
SITE := $(HOME)/public_html/flexbackup
RPM  := /usr/src/redhat
DATE := $(shell date)

commit:
	cvs commit

all: tar rpm webdoc

tag: version commit
	perl -pi -e 's/^Version:        .*/Version:        $(VER)/' flexbackup.lsm.template
	perl -pi -e 's/^Entered-date:   .*/Entered-date:   $(DATE)/' flexbackup.lsm.template
	perl -pi -e 's/%define version .*/%define version $(VER)/' flexbackup.spec
	cvs commit -m "" flexbackup.lsm.template flexbackup.spec
	cvs tag -F $(CVSVER)

tar: tag lsm
	cd /tmp; cvs co -r $(CVSVER) flexbackup; mv flexbackup flexbackup-$(VER)
	tar -C /tmp -z -c -v -X tar.exclude -f $(SITE)/flexbackup-$(VER).tar.gz flexbackup-$(VER)
	cd /tmp; echo yes | cvs release -d flexbackup-$(VER)

rpm: tar
	sudo cp $(SITE)/flexbackup-$(VER).tar.gz $(RPM)/SOURCES
	sudo rpm -ba flexbackup.spec
	cp $(RPM)/RPMS/noarch/flexbackup-$(VER)-1.noarch.rpm $(SITE)
	cp $(RPM)/SRPMS/flexbackup-$(VER)-1.src.rpm $(SITE)
	sudo rpm --rmsource flexbackup.spec
	sudo rm $(RPM)/RPMS/noarch/flexbackup-$(VER)-1.noarch.rpm
	sudo rm $(RPM)/SRPMS/flexbackup-$(VER)-1.src.rpm

webdoc:
	cd /tmp; cvs co -r $(CVSVER) flexbackup
	cp /tmp/flexbackup/CHANGES /tmp/flexbackup/README /tmp/flexbackup/TODO $(SITE)
	cd /tmp; echo yes | cvs release -d flexbackup

lsm: tag
	cd /tmp; cvs co -r $(CVSVER) flexbackup; mv flexbackup flexbackup-$(VER)
	tar -C /tmp -z -c -v -X tar.exclude -f $(SITE)/flexbackup-$(VER).tar.gz flexbackup-$(VER)
	cd /tmp; echo yes | cvs release -d flexbackup-$(VER)
	./sizelsm.perl $(SITE) $(VER)
	cvs commit -m "" flexbackup.lsm
	cvs tag -F $(CVSVER) flexbackup.lsm

version:
	test -n "$(VER)"
