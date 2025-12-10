include make/configs/CONFIGS.mk
include make/Flags.mk
include make/Compiler.mk
include make/Folders.mk
include make/Files.mk

all: $(BIN)

# Assembly files
$(OBJ_DIR)/__%.o: $(SRC_DIR)/%.S
	mkdir -p $(dir $@)
	$(ASM) $(ASM_FLAGS) -c $< -o $@


# C files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) -c $< -o $@


# Rust files TODO: Allow using debug build + gdb
$(OBJ_DIR)/rslib.a: Cargo.toml _src/*.rs
	$(RUST) build --release --target $(RS_TARGET)
	mkdir -p $(OBJ_DIR)
	cp $(RS_LIB_DIR) $(OBJ_DIR)/rslib.a


# Link
$(TARGET): $(OBJ) $(OBJ_DIR)/rslib.a
	mkdir -p $(dir $@)
	$(LD) $(LD_FLAGS) -o $@ $(OBJ) $(OBJ_DIR)/rslib.a
	make disasm
	make full-disasm


# BIN
$(BIN): $(TARGET)
	mkdir -p $(dir $@)
	$(OBJCOPY) -O binary $(TARGET) $(BIN)

# Uimage
UIMAGE = $(BIN_DIR)/$(KERNEL_FILE).uImage
LOAD_ADDR = 0x80000000
ENTRY_ADDR = 0x80000000

uimage: $(BIN)
	mkdir -p $(dir $@)
	mkimage -A arm64 -O linux -T kernel -C none \
	        -a $(LOAD_ADDR) -e $(ENTRY_ADDR) \
	        -d $(BIN) $(UIMAGE)

clean:
	rm -rf $(OBJ_DIR)/* target


disasm: $(OBJ)
	@for o in $(OBJ); do \
	    $(OBJDUMP) -D $$o > $$o.S; \
	done

full-disasm: $(TARGET)
	$(OBJDUMP) -D -S $(TARGET) > $(TARGET).dump.S