SUBDIRS = mfl tui nui

all: $(SUBDIRS)

.PHONY: $(SUBDIRS) clean
MFLAGS += --no-print-directory
$(SUBDIRS):
	@cd $@; $(MAKE) $(MFLAGS) 

clean:
	@for i in $(SUBDIRS); do \
	echo "Clearing in $$i..."; \
	(cd $$i; $(MAKE) $(MFLAGS) $(MYMAKEFLAGS) clean); done
