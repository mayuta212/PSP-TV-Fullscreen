TARGET = PSPTVFullscreen
OBJS = main.o lcdc_imports.o reg_imports.o
CFLAGS = -O2 -G0 -Wall -Wextra -std=c99 -fno-pic -fno-builtin
ASFLAGS = -G0 -O2
BUILD_PRX = 1
PRX_EXPORTS = exports.exp
USE_KERNEL_LIBS = 1
USE_KERNEL_LIBC = 1
PSP_FW_VERSION = 661
LDFLAGS = -nostartfiles
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
