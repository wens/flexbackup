
CVSVER := $(shell echo v$(VER) | sed -e 's/\./_/g')

commit:
	cvs commit

all: tar rpm

tag: version commit
	cvs tag -F $(CVSVER)

tar: version commit tag
	cd /tmp; cvs co -r $(CVSVER) flexbackup; mv flexbackup flexbackup-$(VER)
	tar -C /tmp -z -c -v -X tar.exclude -f ../flexbackup-$(VER).tar.gz flexbackup-$(VER)
	cd /tmp; echo yes | cvs release -d flexbackup-$(VER)

rpm: version commit tar
	sudo cp ../flexbackup-$(VER).tar.gz /usr/src/redhat/SOURCES
	sudo rpm -ba flexbackup.spec
	sudo cp /usr/src/redhat/RPMS/noarch/flexbackup* ..
	sudo cp /usr/src/redhat/SRPMS/flexbackup* ..
	sudo rpm --rmsource --clean flexbackup.spec

version:
	test -n "$(VER)"
