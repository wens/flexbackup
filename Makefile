#
# $Id$
# $Name$
#

CVSVER := $(shell echo v$(VER) | sed -e 's/\./_/g')
SITE := ../web
RPM  := /net/rpm
DATE := $(shell date)
CVS := cvs -d :ext:edwinh@cvs.sourceforge.net:/cvsroot/flexbackup

commit:
	$(CVS) commit

all: tar rpm lsm

tag: version commit

	$(CVS) tag -F $(CVSVER)

lsm: version
	cp flexbackup.lsm.template flexbackup.lsm
	perl -pi -e 's/VERSION/$(VER)/' flexbackup.lsm
	perl -pi -e 's/DATE/$(DATE)/' flexbackup.lsm

tar: version tag lsm
	cd /tmp; $(CVS) export -r $(CVSVER) flexbackup; mv flexbackup flexbackup-$(VER)
	cd /tmp/flexbackup-$(VER); mv Makefile.dist Makefile
	cd /tmp/flexbackup-$(VER); mv flexbackup.spec.template flexbackup.spec
	cd /tmp/flexbackup-$(VER); perl -pi -e 's/%define version.*/%define version $(VER)/' flexbackup.spec
	cd $(SITE); makefaq.py -o faq.html
	cd $(SITE); makefaq.py -c text -o /tmp/flexbackup-$(VER)/FAQ
	tar -C /tmp -z -c -v -X tar.exclude -f $(SITE)/tarball/flexbackup-$(VER).tar.gz flexbackup-$(VER)
	ln -snf flexbackup-$(VER).tar.gz $(SITE)/tarball/flexbackup-latest.tar.gz
	cp /tmp/flexbackup-$(VER)/CHANGES /tmp/flexbackup-$(VER)/README /tmp/flexbackup-$(VER)/TODO $(SITE)
	./flexbackup -h > $(SITE)/usage.txt
	cp ./flexbackup.conf $(SITE)/flexbackup.conf.txt
	rm -rf /tmp/flexbackup-$(VER)

rpm: version tar
	rpmbuild -ta $(SITE)/tarball/flexbackup-$(VER).tar.gz
	cp -p $(RPM)/RPMS/noarch/flexbackup-$(VER)-1.noarch.rpm $(SITE)/RPMS
	cp -p $(RPM)/SRPMS/flexbackup-$(VER)-1.src.rpm $(SITE)/RPMS
	rm $(RPM)/RPMS/noarch/flexbackup-$(VER)-1.noarch.rpm
	rm $(RPM)/SRPMS/flexbackup-$(VER)-1.src.rpm

rsync:
	rsync --archive --verbose --cvs-exclude --delete-excluded --delete $(SITE)/ edwinh.org:public_html/flexbackup
	rsync --archive --verbose --cvs-exclude --delete-excluded --delete $(SITE)/ flexbackup.sourceforge.net:/home/groups/f/fl/flexbackup/htdocs

upload: version lsm
	test -e $(SITE)/tarball/flexbackup-$(VER).tar.gz
	ncftpput ibiblio.org /incoming/Linux $(SITE)/tarball/flexbackup-$(VER).tar.gz
	ncftpput ibiblio.org /incoming/Linux flexbackup.lsm
	ncftpput incoming.redhat.com /libc6 $(SITE)/RPMS/flexbackup-$(VER)-1.noarch.rpm
	ncftpput incoming.redhat.com /libc6 $(SITE)/RPMS/flexbackup-$(VER)-1.src.rpm

version:
	test -n "$(VER)"
