PACKAGE = WMSigner
VERSION = 1.0.0
SHELL = /bin/sh

PREFIX = ${prefix}/usr/local
bindir = $(PREFIX)/bin
mandir = $(PREFIX)/man/man1

# all dirs
DIRS = $(bindir) $(mandir)

# INSTALL scripts
INSTALL		= install -p --verbose
INSTALL_BIN     = $(INSTALL) -m 755
INSTALL_DIR     = $(INSTALL) -m 755 -d
INSTALL_DATA    = $(INSTALL) -m 644

all: cmdbase.cpp crypto.cpp md4.cpp rsalib1.cpp signer.cpp wmsigner.cpp
	/usr/bin/g++ -static cmdbase.cpp crypto.cpp md4.cpp rsalib1.cpp signer.cpp wmsigner.cpp -o WMSigner
	/bin/chmod g+x,o+x WMSigner

install: WMSigner WMSigner.ini
	for dir in $(DIRS) ; do \
	  $(INSTALL_DIR) $$dir ; \
	done
	$(INSTALL_BIN) WMSigner $(bindir)
	$(INSTALL_DATA) WMSigner.ini $(bindir)/
	$(INSTALL_DATA) WMSigner.1 $(mandir)

clean:
	rm -f WMSigner