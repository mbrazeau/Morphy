SUBDIRS = mfl tui nui
ALLFILES = $(addsuffix /*.c, $(SUBDIRS)) $(addsuffix /*.cpp, $(SUBDIRS))
ifndef VERBOSE
SILENT=@
NICECSCOUTPUT=@echo " CScope for $(SUBDIRS)"
endif

all: $(SUBDIRS)

.PHONY: $(SUBDIRS) clean

MFLAGS += --no-print-directory

$(SUBDIRS): cscope.out
	@cd $@; $(MAKE) $(MFLAGS)

cscope.out: $(wildcard $(ALLFILES))
	$(NICECSCOUTPUT)
	$(SILENT)cscope -b -u $(patsubst %,-s %,$(SUBDIRS)) -s ../ncl-2.1.17/ncl
    
clean:
	-@rm cscope.out
	@for i in $(SUBDIRS); do \
	echo "Cleaning $$i..."; \
	(cd $$i; $(MAKE) $(MFLAGS) $(MYMAKEFLAGS) clean); done
