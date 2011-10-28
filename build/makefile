#FIXME	get the path of the root
ROOT_DIR := $(PWD)/../
DIRS= $(ROOT_DIR)/common/logging \
      $(ROOT_DIR)/common/base \
      $(ROOT_DIR)/message/mock_API \
      $(ROOT_DIR)/message/build \
      $(ROOT_DIR)/testapp

COMPILE_OPT = -D_GNU_SOURCE -D_THREAD_SAFE -O2 -g -Wall -Wextra -I$(ROOT_DIR) #-Werror -Wfatal-errors 
#COMPILE_OPT = -pg -O -g -fprofile-arcs -ftest-coverage

#export GCOVFLAGS = 
export CFLAGS = $(COMPILE_OPT)
export CXXFLAGS = $(COMPILE_OPT)
export CC=gcc
export CXX=g++
.PHONY : all $(DIRS)
all: $(DIRS)
$(DIRS):
	@ $(MAKE) -C $@
	
$(ROOT_DIR)/common/base: $(ROOT_DIR)/common/logging
$(ROOT_DIR)/message/mock_API: $(ROOT_DIR)/common/base
$(ROOT_DIR)/message/build : $(ROOT_DIR)/message/mock_API
$(ROOT_DIR)/testapp : $(ROOT_DIR)/message/mock_API

#	for d in $(DIRS); do  $(MAKE) -C  $$d; done;
#	cd ..; pwd; rm -rf cov/*; rm cov.info; lcov -c -d . -o cov.info; genhtml -o cov cov.info

.PHONY: clean
clean:
	for d in $(DIRS); do $(MAKE) -C $$d clean; done;
#	rm -rf ../cov/*
#	rm ../cov.info
