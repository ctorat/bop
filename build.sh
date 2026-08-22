#!/bin/bash
#
# Copyright (c) 2026, Apollo Telephone Laboratories.
# Provided under the BSD-3 clause.
#

. dev/build.env

BUILD_DIRS=core
BOP_BUILD=$BUILD_DIRS bop build
