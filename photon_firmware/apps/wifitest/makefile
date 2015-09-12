mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(patsubst %/,%,$(dir $(mkfile_path)))
APPNAME := $(notdir $(mkfile_dir:/=))

all: build flash

include ../makefile
