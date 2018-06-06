# -*- Mode: Makefile; -*-
#
# See COPYRIGHT in top-level directory.
#

TESTS = \
	array_to_list \
	array_to_scdlist

XFAIL_TESTS =

check_PROGRAMS = $(TESTS)
noinst_PROGRAMS = $(TESTS)

include $(top_srcdir)/test/Makefile.mk

array_to_list_SOURCES = array_to_list.c
array_to_scdlist_SOURCES = array_to_scdlist.c

array_to_list_CFLAGS =
array_to_scdlist_CFLAGS = -fopenmp

array_to_list_LDFLAGS = -pthread
array_to_scdlist_LDFLAGS = -fopenmp
