include make/Folders.mk

OPT_LEVEL   ?= -O2
DEFINES     ?=

MARCH       ?= armv8-a
MCPU        ?= cortex-a53+simd
RS_TARGET	= aarch64-unknown-none

ASM_FLAGS   = $(DEFINES)
C_FLAGS     = $(OPT_LEVEL) $(DEFINES) -std=gnu23 -Wall -Wextra -Werror -ffreestanding -nostdlib -nostdinc -nostartfiles -x c -I$(INCLUDE_DIR) -march=$(MARCH) -mcpu=$(MCPU)
LD_FLAGS    = -T linker.ld -Map $(MAP)

$(OBJ_DIR)/drivers/%.o: C_FLAGS += -DDRIVERS
$(OBJ_DIR)/kernel/%.o: C_FLAGS += -DKERNEL
$(OBJ_DIR)/lib/%.o: C_FLAGS += -DLIB
$(OBJ_DIR)/boot/%.o: C_FLAGS += -DBOOT