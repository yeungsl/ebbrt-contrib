SUBDIRS := $(wildcard */.)

all: Release

.PHONY: all Debug Release clean $(SUBDIRS)


$(SUBDIRS):
	$(MAKE) -C $@

Debug:
	for dir in $(SUBDIRS); do \
    $(MAKE) Debug -C $$dir; \
  done

Release:
	for dir in $(SUBDIRS); do \
    $(MAKE) Release -C $$dir; \
  done

clean:
	for dir in $(SUBDIRS); do \
    $(MAKE) clean -C $$dir; \
  done
