
CVSVER := $(shell echo v$(VER) | sed -e 's/\./_/g')
SITE := $(HOME)/public_html/flexbackup
RPM  := /usr/src/redhat

commit:
	cvs commit

all: tar rpm

tag: version commit
	cvs tag -F $(CVSVER)

tar: version commit tag
	cd /tmp; cvs co -r $(CVSVER) flexbackup; mv flexbackup flexbackup-$(VER)
	tar -C /tmp -z -c -v -X tar.exclude -f $(SITE)/flexbackup-$(VER).tar.gz flexbackup-$(VER)
	cd /tmp; echo yes | cvs release -d flexbackup-$(VER)

rpm: version commit tar
	sudo cp $(SITE)/flexbackup-$(VER).tar.gz $(RPM)/SOURCES
	sudo rpm -ba flexbackup.spec
	cp $(RPM)/RPMS/noarch/flexbackup-$(VER)-1.noarch.rpm $(SITE)
	cp $(RPM)/SRPMS/flexbackup-$(VER)-1.src.rpm $(SITE)
	sudo rpm --rmsource flexbackup.spec
	sudo rm $(RPM)/RPMS/noarch/flexbackup-$(VER)-1.noarch.rpm
	sudo rm $(RPM)/SRPMS/flexbackup-$(VER)-1.src.rpm

version:
	test -n "$(VER)"
