
EXTRA_DIST = README dhahelper.c

CFLAGS = -O2 -Wall $(LINUX_CFLAGS)

KVERSION = $(shell $(SHELL) -c 'uname -r')
moddir = /lib/modules/$(KVERSION)/misc

if AMM_USE_LINUX
KERNEL_MODULE = dhahelper.o
endif

noinst_HEADERS = dhahelper.h

EXTRA_PROGRAMS = test
test_SOURCES = test.c

dhahelper.o: $(srcdir)/dhahelper.c
	$(CC) $(CFLAGS) -o $@ -c $<

nodes:
	$(MKNOD) -m 666 /dev/dhahelper c 180 0

all: $(KERNEL_MODULE)

#install-exec-local: $(KERNEL_MODULE)
#@AMM_USE_LINUX_TRUE@	@$(NORMAL_INSTALL)
#@AMM_USE_LINUX_TRUE@	$(mkinstalldirs) $(DESTDIR)$(moddir)
#@AMM_USE_LINUX_TRUE@	@list='$(KERNEL_MODULE)'; \
#@AMM_USE_LINUX_TRUE@	for p in $$list; do \
#@AMM_USE_LINUX_TRUE@	  if test -f $$p; then \
#@AMM_USE_LINUX_TRUE@	    echo "$(INSTALL) -o root -g root -m 644 $$p $(DESTDIR)$(moddir)/$$p"; \
#@AMM_USE_LINUX_TRUE@	    $(INSTALL) -o root -g root -m 644 $$p $(DESTDIR)$(moddir)/$$p; \
#@AMM_USE_LINUX_TRUE@	  else :; fi; \
#@AMM_USE_LINUX_TRUE@	done; \
#@AMM_USE_LINUX_TRUE@	$(DEPMOD) -a
#@AMM_USE_LINUX_TRUE@	if test ! -c /dev/dhahelper; then \
#@AMM_USE_LINUX_TRUE@	  $(MAKE) nodes; \
#@AMM_USE_LINUX_TRUE@	fi


#uninstall-local:
#@AMM_USE_LINUX_TRUE@	@$(NORMAL_UNINSTALL)
#@AMM_USE_LINUX_TRUE@	@list='$(KERNEL_MODULE)'; \
#@AMM_USE_LINUX_TRUE@	for p in $$list; do \
#@AMM_USE_LINUX_TRUE@	  echo "rm -f $(DESTDIR)$(moddir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
#@AMM_USE_LINUX_TRUE@	  rm -f $(DESTDIR)$(moddir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
#@AMM_USE_LINUX_TRUE@	done

MAINTAINERCLEfANFILES = Makefile.in
