CELL_MK_DIR = $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

PEXPORTPICKUP = ppu-lv2-prx-exportpickup

CRT_HEAD += $(shell ppu-lv2-gcc -print-file-name'='ecrti.o)
CRT_HEAD += $(shell ppu-lv2-gcc -print-file-name'='crtbegin.o)
CRT_TAIL += $(shell ppu-lv2-gcc -print-file-name'='crtend.o)
CRT_HEAD += $(shell ppu-lv2-gcc -print-file-name'='ecrtn.o)


PPU_SRCS  = mem.c misc.c png_dec.c blitting.c scroller.c starfield.c main.c

#FOR_DEBUG:
PPU_SRCS += network.c

PPU_INCDIRS = -I ./inc
PPU_PRX_TARGET = ps3_vsh_menu.prx

#PPU_PRX_LDFLAGS = -L ./lib -Wl, --strip-unused-data
PPU_PRX_LDFLAGS = -L ./lib --gc-sections --as-needed
PPU_PRX_STRIP_FLAGS = -s

PPU_PRX_LDLIBS  = -lfs_stub \
                  -lrtc_stub \
                  -lstdc_export_stub \
                  -lsysPrxForUser_export_stub \
                  -lvsh_export_stub \
                  -lpaf_export_stub \
                  -lvshmain_export_stub \
                  -lvshtask_export_stub \
                  -lallocator_export_stub \
                  -lsdk_export_stub \
                  -lxsetting_export_stub \
                  -lsys_io_export_stub \
                  -lpngdec_ppuonly_export_stub
#FOR_DEBUG:
PPU_PRX_LDLIBS += -lnet_stub

PPU_CFLAGS += -Os -ffunction-sections -fdata-sections \
              -fno-builtin-printf -nodefaultlibs -std=gnu99 \
              -Wno-shadow -Wno-unused-parameter


PPU_CFLAGS += -DDEBUG

#PPU_CFLAGS += -DHAVE_PNG_FONT
#PPU_CFLAGS += -DHAVE_XBM_FONT
PPU_CFLAGS += -DHAVE_SYS_FONT

PPU_CFLAGS += -DHAVE_STARFIELD
PPU_CFLAGS += -DHAVE_SSCROLLER


CLEANFILES = ./$(PPU_SPRX_TARGET)

all:
	$(MAKE) $(PPU_OBJS_DEPENDS)
	$(PPU_PRX_STRIP) --strip-debug --strip-section-header $(PPU_PRX_TARGET)
	
include $(CELL_MK_DIR)/sdk.target.mk
