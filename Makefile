# Makefile - Bare-metal STM32F103C8T6 (Blue Pill)
# Toolchain: arm-none-eabi-gcc
# Flash via: st-flash or openocd

PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
SIZE    = $(PREFIX)size

TARGET  = firmware

# Compiler flags: C89, no stdlib, thumb, cortex-m3
CFLAGS  = -std=c89 -Wall -Wextra -Os \
          -mcpu=cortex-m3 -mthumb \
          -ffreestanding -nostdlib \
          -fno-common

LDFLAGS = -T linker.ld -nostdlib -Wl,--gc-sections
LDLIBS  = -lgcc

SRCS    = startup.c main.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean flash

all: $(TARGET).bin
	$(SIZE) $(TARGET).elf

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: $(TARGET).bin
	st-flash write $(TARGET).bin 0x08000000

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin