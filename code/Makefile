MC1322X := .

KERNEL := $(MC1322X)/kernel
UTILS := $(MC1322X)/utils
NSEG := $(MC1322X)/nseg
LIBMC1322X := $(MC1322X)/lib

default: debug all
.PHONY: default

CROSS_COMPILE := arm-linux-

LINKERSCRIPT := $(MC1322X)/mc1322x.lds

UTILSOBJS=$(patsubst $(UTILS)/%.c,$(OBJDIR)/%.o,$(wildcard $(UTILS)/*.c))
KERNELOBJS=$(patsubst $(KERNEL)/%.c,$(OBJDIR)/%.o,$(wildcard $(KERNEL)/*.c))
NSEGOBJS=$(patsubst $(NSEG)/%.c,$(OBJDIR)/%.o,$(wildcard $(NSEG)/*.c))
SRCOBJS += $(MC1322X)/src/default_lowlevel.o $(ISR) $(START) $(SWI) 
ISR = $(MC1322X)/src/isr.o
SWI = $(MC1322X)/src/swi.o

PLATFORM_RELFLAGS = -fno-strict-aliasing  -fno-common -ffixed-r8 -ffunction-sections -msoft-float -Wcast-align -Wall
PLATFORM_CPPFLAGS = -march=armv4t -mlong-calls -mtune=arm7tdmi-s -DCONFIG_ARM -D__ARM__ -mthumb-interwork 
THUMB_FLAGS= -mthumb -mcallee-super-interworking

#########################################################################

AS	= $(CROSS_COMPILE)as
LD	= $(CROSS_COMPILE)ld
CC	= $(CROSS_COMPILE)gcc
CPP	= $(CC) -E
AR	= $(CROSS_COMPILE)ar
NM	= $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

RELFLAGS= $(PLATFORM_RELFLAGS)
DBGFLAGS= -g -DDEBUG
OPTFLAGS= -Os #-fomit-frame-pointer
OBJCFLAGS += --gap-fill=0xff

gccincdir := $(shell $(CC) -print-file-name=include)

CPPFLAGS := $(DBGFLAGS) $(OPTFLAGS) $(RELFLAGS)		\
	-D__KERNEL__ -DTEXT_BASE=$(TEXT_BASE)		\
	-std=gnu99 -fno-builtin -ffreestanding -isystem	\
	$(gccincdir) -pipe $(PLATFORM_CPPFLAGS)

CFLAGS := $(CPPFLAGS) -Wall -Wstrict-prototypes -Wcast-align -Wextra -Wshadow -Werror
CFLAGS += -I$(LIBMC1322X)/include -I$(MC1322X)/src -I$(MC1322X)/kernel 

AFLAGS_DEBUG := -Wa,-gstabs
AFLAGS := $(AFLAGS_DEBUG) -D__ASSEMBLY__ $(CPPFLAGS)

OBJDIR := $(MC1322X)/obj

$(OBJDIR):
	mkdir $(OBJDIR)


LIBOBJS = $(patsubst %.c,%.o,$(wildcard $(LIBMC1322X)/*.c))

$(LIBMC1322X)/libmc1322x.a: $(LIBOBJS) 
	$(AR) rcs $(LIBMC1322X)/libmc1322x.a $(LIBOBJS)


START = $(MC1322X)/src/start-romvars.o		

include $(wildcard $(OBJDIR)/*.d)
include $(wildcard $(MC1322X)/src/*.d)
include $(wildcard $(MC1322X)/lib/*.d)

define FINALIZE_DEPENDENCY
cp $(@:.o=.d) $(@:.o=.$$$$); \
sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
    -e '/^$$/ d' -e 's/$$/ :/' < $(@:.o=.$$$$) >> $(@:.o=.d); \
rm -f $(@:.o=.$$$$)
endef

$(START): $(START:.o=.s)
	$(CC) $(AFLAGS) -c -o $@ $<
$(MC1322X)/src/start-romvars.s:	$(MC1322X)/src/start.S
	$(CPP) $(AFLAGS) -DUSE_INTS -DUSE_ROM_VECTS -DUSE_ROM_VARS -o $@ $<

$(ISR): $(ISR:.o=.c)
	$(CC) $(CFLAGS) -MMD $< -c -o $@
	$(FINALIZE_DEPENDENCY)

$(SWI): $(SWI:.o=.c)
	$(CC) $(CFLAGS) -MMD $< -c -o $@
	$(FINALIZE_DEPENDENCY)


$(MC1322X)/src/src.a: $(MC1322X)/src/src.a($(SRCOBJS))

kernel.elf:	$(START) $(ISR) $(SWI) $(SRCOBJS) $(KERNELOBJS) $(LINKERSCRIPT) $(LIBMC1322X)/libmc1322x.a $(MC1322X)/src/src.a
	$(CC) \
	-T $(LINKERSCRIPT) -nostartfiles -static -Wl,-Map=kernel.map -export-dynamic \
	$(KERNELOBJS) -o $@ $(START) --start-group $(MC1322X)/src/src.a $(LIBMC1322X)/libmc1322x.a --end-group

utils/%.elf:	utils/%.o $(START) $(ISR) $(SWI) $(SRCOBJS) $(KERNELOBJS) $(LINKERSCRIPT) $(LIBMC1322X)/libmc1322x.a $(MC1322X)/src/src.a
	$(CC) \
	-T $(LINKERSCRIPT) -nostartfiles -static -Wl,-Map=utils/$*.map -export-dynamic \
	$(OBJDIR)/pinio.o $(OBJDIR)/put.o $< -o $@ $(START) --start-group $(MC1322X)/src/src.a $(LIBMC1322X)/libmc1322x.a --end-group


%.srec:		%.elf
		$(OBJCOPY) ${OBJCFLAGS} -O srec $< $@

%.ihex: 	%.elf
		$(OBJCOPY) ${OBJCFLAGS} -O ihex $< $@

%/%.bin:		%/%.elf
		$(OBJCOPY) ${OBJCFLAGS} -O binary $< $@

%.bin:		%.elf
		$(OBJCOPY) ${OBJCFLAGS} -O binary $< $@

%.dis:		%.elf
		$(OBJDUMP) -SD $< > $@

%.s: %.S
	$(CPP) $(AFLAGS) -o $@ $<
%.o: %.S
	$(CC) $(AFLAGS) -MMD -c -o $@ $<
	$(FINALIZE_DEPENDENCY)
%.o:	%.c
	$(CC) $(CFLAGS) $(THUMB_FLAGS) -MMD -c -o $@ $<
	$(FINALIZE_DEPENDENCY)

$(OBJDIR)/%.s:	%.S
	$(CPP) $(AFLAGS) -o $@ $<
$(OBJDIR)/%.o:	%.S
	$(CC) $(AFLAGS) -MMD -c -o $@ $<
	$(FINALIZE_DEPENDENCY)
$(OBJDIR)/%.o:	%.c
	$(CC) $(CFLAGS) $(THUMB_FLAGS) -MMD -c -o $@ $<
	$(FINALIZE_DEPENDENCY)

%/%.o:	%.c
	$(CC) $(CFLAGS) $(THUMB_FLAGS) -MMD -c -o $@ $<
	$(FINALIZE_DEPENDENCY)

$(filter %.o,$(KERNELOBJS)): $(OBJDIR)/%.o: $(KERNEL)/%.c 
	$(CC) $(CFLAGS) $(THUMB_FLAGS) -MMD -c -o $(OBJDIR)/"`basename $@`" $<
	-$(FINALIZE_DEPENDENCY)

clean:
	find $(MC1322X) \
		\( -name 'core' -o -name '*.bak' -o -name '*~' \
		-o -name '*.o'  -o -name '*.a' -o -name '*.obj' -o -name '*.elf'  -o -name '*.s' -o -name '*.map' \
                -o -name 'obj_*_board' -o -name '.depend' -o -name '*.bin' -o -name '*.dis' -o -name '*.d' -o -name '*.srec' \) -print \
		| xargs rm -rf

distclean:	clean

.PHONY: clean distclean debug

debug: 
	echo KERNELOBJS: $(KERNELOBJS) 

all: kernel.bin utils/dflash.bin

.PHONY: all 

