TARGET = kved
DEBUG = 1
OPT = -Og
BUILD_DIR = build
C_SOURCES = \
    kved.c \
    kved_cpu.c \
    ./port/simul/port_flash.c \
	./test/kved_test.c  \
	./test/kved_test_main.c

#PREFIX = arm-none-eabi-
PREFIX =

CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size

C_DEFS =  \
    -DKVED_FLASH_WORD_SIZE=8 

C_INCLUDES =  \
    -I. 

CFLAGS = $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
    CFLAGS += -g 
    C_DEFS += -DKVED_DEBUG
endif

LIBS = 
LIBDIR = 
LDFLAGS =  $(LIBDIR) $(LIBS) -Wl,--gc-sections

all: $(BUILD_DIR)/$(TARGET).elf 

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR):
	mkdir $@		

clean:
	-rm -fR $(BUILD_DIR)
  
-include $(wildcard $(BUILD_DIR)/*.d)

