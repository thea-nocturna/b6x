# Targets
EMULATOR = build/b6x
ZPTOOL = build/b6xzp

# Installation path
PREFIX = /usr/local

# Version
V_MAJ = 0
V_MIN = 0
V_FIX = 1

VERSION := $(shell printf "0x%04x"  \
               $$(( ($(V_MAJ) << 12) | ($(V_MIN) << 6) | $(V_FIX) )))

# Compiler related
CC = gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter \
         -Iinclude -std=c99 -DVERSION=$(VERSION)