
CVSVER := $(shell echo v$(VER) | sed -e 's/\./_/g')
SITE := $(HOME)/public_html/flexbackup
RPM  := /usr/src/redhat

commit:
	cvs commit

all: tar rpm webdoc

tag: version commit
	cvs tag -F $(CVSVER)

tar: tag
	cd /tmp; cvs co -r $(CVSVER) flexbackup; mv flexbackup flexbackup-$(VER)
	tar -C /tmp -z -c -v -X tar.exclude -f $(SITE)/flexbackup-$(VER).tar.gz flexbackup-$(VER)
	cd /tmp; echo yes | cvs release -d flexbackup-$(VER)

rpm: tar
	perl -pi -e 's/%define version .*/%define version $(VER)/' flexbackup.spec
	cvs commit -m"version to $(VER)" flexbackup.spec
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


version:
	test -n "$(VER)"
