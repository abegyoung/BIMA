
top_srcdir = .

SUBDIRS = lib

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

depend:
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir depend; \
	done

proto:
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir proto; \
	done

clean:
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir clean; \
	done
	find . -name "Makefile.bak" | xargs rm -f

install:
	(cd lib; make install)
	(cd include; make install)

realclean:
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir realclean; \
	done
	find . -name "core.*" | xargs rm -f

