
CVSVER := $(shell echo v$(VER) | sed -e 's/\./_/g')

commit:
	cvs commit

tag: version commit
	cvs tag -F $(CVSVER)

tar: version commit tag
	cd /tmp; cvs co -r $(CVSVER) flexbackup; mv flexbackup flexbackup-$(VER)
	tar -C /tmp -z -c -v -X tar.exclude -f ../flexbackup-$(VER).tar.gz flexbackup-$(VER)
	cd /tmp; echo yes | cvs release -d flexbackup-$(VER)

rpm: version commit tar


version:
	test -n "$(VER)"
