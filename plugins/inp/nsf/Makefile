#
# Make input plugins for dcplaya
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.1 2003-04-08 20:46:46 ben Exp $
#

LEZ=yes

NFS_DIR		:= nosefart-1.92c-mls
NSF_INCS 	:= . machine cpu/nes6502 sndhrdw
NSF_LIB		:= $(NFS_DIR)/lib$(NFS_DIR).a

LEF_TARGET_EXTRA_DEP := $(NSF_LIB)
LEF_EXTRA := -L$(NFS_DIR) -l$(NFS_DIR)

KOS_LOCAL_CFLAGS := -DHOST_LITTLE_ENDIAN -DDCPLAYA=1 -DNSF_PLAYER=1

KOS_LOCAL_INCS := -I../../../include\
 $(foreach i,$(NSF_INCS),-I$(NFS_DIR)/src/$(i))

OPTIMIZE = $(OPTIMIZE_FOR_SPEED)


include ../../../Makefile.inc

.PHONY: $(NSF_LIB)
$(NSF_LIB):
	$(MAKE) -C $(NFS_DIR) -f Makefile.dcp

