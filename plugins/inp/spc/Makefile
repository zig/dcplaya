#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.4 2003-04-19 23:53:48 vincentp Exp $
#

LEZ=yes

SPC_DIR		:= spc
SPC_INCS 	:= . libspc
SPC_LIB		:= $(SPC_DIR)/lib$(SPC_DIR).a

LEF_TARGET_EXTRA_DEP := $(SPC_LIB)
LEF_EXTRA := -L$(SPC_DIR) -l$(SPC_DIR)

KOS_LOCAL_CFLAGS := -DHOST_LITTLE_ENDIAN -DDCPLAYA=1 -DSPC_PLAYER=1

KOS_LOCAL_INCS := -I../../../include\
 $(foreach i,$(SPC_INCS),-I$(SPC_DIR)/$(i))

OPTIMIZE = $(OPTIMIZE_FOR_SPEED)


include ../../../Makefile.inc

.PHONY: $(SPC_LIB)
$(SPC_LIB):
	$(MAKE) -C $(SPC_DIR) -f Makefile.dcp

