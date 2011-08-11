#FIXME	get the path of the root
ROOT_DIR=../
DIRS= $(ROOT_DIR)/common/logging \
      $(ROOT_DIR)/common/base \
      $(ROOT_DIR)/message/mock_API \
      $(ROOT_DIR)/message/build \
      $(ROOT_DIR)/testapp

COMPILE_OPT =  -pg -O2 -g -Wall -Wextra #-Werror -Wfatal-errors 
#COMPILE_OPT = -pg -O -g -fprofile-arcs -ftest-coverage

#export GCOVFLAGS = 
export CFLAGS = $(COMPILE_OPT)
export CXXFLAGS = $(COMPILE_OPT)
export CC=gcc
export CXX=g++

all: $(DIRS)
	for d in $(DIRS); do  $(MAKE) -C $$d; done;
#	cd ..; pwd; rm -rf cov/*; rm cov.info; lcov -c -d . -o cov.info; genhtml -o cov cov.info

.PHONY: clean
clean: $(DIRS)
	for d in $(DIRS); do $(MAKE) clean -C $$d; done;
#	rm -rf ../cov/*
#	rm ../cov.info
