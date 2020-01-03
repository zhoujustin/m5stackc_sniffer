#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := m5_packetsmonitor

SOLUTION_PATH ?= $(abspath $(shell pwd))/

include $(SOLUTION_PATH)/components/component_conf.mk
include $(IDF_PATH)/make/project.mk

