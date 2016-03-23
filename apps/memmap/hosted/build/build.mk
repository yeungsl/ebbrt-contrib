MYDIR := $(dir $(lastword $(MAKEFILE_LIST)))

app_sources := \
	memmap.cc \
	Printer.cc

target := memmap

include $(abspath $(EBBRT_SRCDIR)/apps/ebbrthosted.mk)
