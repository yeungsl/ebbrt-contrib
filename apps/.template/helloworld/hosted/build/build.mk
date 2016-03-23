MYDIR := $(dir $(lastword $(MAKEFILE_LIST)))

app_sources := \
	{{ title }}.cc \
	Printer.cc

target := {{ title }}

include $(abspath $(EBBRT_SRCDIR)/apps/ebbrthosted.mk)
