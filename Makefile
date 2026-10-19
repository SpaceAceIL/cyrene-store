TOOLCHAIN := /home/space/ps4-toolchain/OpenOrbis/PS4Toolchain
LINK_X    := /home/space/ps4-toolchain/OpenOrbis/PS4Toolchain/link.x
CC        := /usr/lib/llvm-18/bin/clang
LD        := /usr/lib/llvm-18/bin/ld.lld
FSELF     := /home/space/ps4-toolchain/OpenOrbis/PS4Toolchain/bin/linux/create-fself

TARGET  := m3u8-ps4
BUILD   := build-ps4
STAGE   := $(BUILD)/stage
OBJS    := $(BUILD)/src/main.o $(BUILD)/src/http.o $(BUILD)/src/catalog.o $(BUILD)/src/install.o $(BUILD)/src/video.o $(BUILD)/src/input.o

# Official OpenOrbis sample flags (hello_world/Makefile)
CFLAGS  := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables \
           -O2 -Wall -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include

LDFLAGS := -m elf_x86_64 -pie --script $(LINK_X) --eh-frame-hdr \
           -L$(TOOLCHAIN)/lib

LIBS    := -lc -lkernel -lSceSystemService -lSceVideoOut -lScePad -lSceUserService -lSceNet -lSceSsl -lSceHttp -lSceAppInstUtil

.PHONY: all clean

all: $(STAGE)/eboot.bin

$(BUILD)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS) $(TOOLCHAIN)/lib/crt1.o
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) $(TOOLCHAIN)/lib/crt1.o -o $@

$(STAGE)/eboot.bin: $(BUILD)/$(TARGET).elf
	@mkdir -p $(STAGE)
	$(FSELF) -in=$< -out=$(BUILD)/$(TARGET).oelf --eboot $@ --paid 0x3800000000000011
	@[ -f channels.m3u ] && cp -f channels.m3u $(STAGE)/ || true

clean:
	rm -rf $(BUILD)
