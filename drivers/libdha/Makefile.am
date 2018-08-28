SUBDIRS = bin kernelhelper oth sysdep

noinst_HEADERS = AsmMacros.h libdha.h
EXTRA_DIST = README pci_db2c.awk

AWK_GENERATED = pci_dev_ids.c pci_ids.h pci_names.c pci_names.h pci_vendors.h
DHA_SRC = cpu_flush.c irq.c libdha.c mtrr.c pci.c pci_names.c mmi.c ports.c

if AMM_LINK_SHARED
lib_LTLIBRARIES = libaviplaydha.la
else
noinst_LTLIBRARIES = libdha.la
endif

if AMM_USE_VIDIX
libaviplaydha_la_SOURCES = $(DHA_SRC)
libdha_la_SOURCES = $(DHA_SRC)
endif
libaviplaydha_la_LDFLAGS = -release $(AVIFILE_MAJOR_VERSION).$(AVIFILE_MINOR_VERSION) -version-info 0:$(AVIFILE_MICRO_VERSION):0

EXTRA_PROGRAMS = test

test_SOURCES = test.c
test_LDADD = $(lib_LTLIBRARIES) $(noinst_LTLIBRARIES)

$(AWK_GENERATED): $(srcdir)/pci_db2c.awk $(srcdir)/oth/pci.db
	$(AWK) -f $(srcdir)/pci_db2c.awk $(srcdir)/oth/pci.db

$(srcdir)/pci.c: $(AWK_GENERATED)
$(srcdir)/pci_names.c: $(srcdir)/pci_names.h $(srcdir)/pci_dev_ids.c
$(srcdir)/pci_ids.h: $(srcdir)/pci_vendors.h

CLEANFILES = $(AWK_GENERATED)
MAINTAINERCLEANFILES = Makefile.in
