include config.mk

SELF = Makefile config.mk

SRCS = src/core/uxn.c \
	   src/dev/stk.c src/dev/init.c src/dev/dbg.c \
	   src/dev/rom.c src/dev/vdp.c src/dev/ctl.c

ifndef $(BACKEND)
	BACKEND = minifb_x11
endif

ifeq ($(BACKEND), minifb_x11)
	LDFLAGS = -lminifb -lX11 -lGL
	SRCS += src/main/minifb.c
else 
ifeq ($(BACKEND), minifb_win32)
	LDFLAGS = lib/minifb/libminifb.a -lgdi32 -lopengl32 -lwinmm
	CFLAGS += -isystem ./lib/minifb
	SRCS += src/main/minifb.c
else
	ERR = $(error Unsupported backend: $(BACKEND))
endif
endif

OBJS = $(patsubst src/%.c, build/%.o, $(SRCS))
DEPS = $(patsubst build/%.o, build/%.d, $(OBJS))

all: $(ERR) b6x b6xzp

b6x: $(EMULATOR)
$(EMULATOR): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(EMULATOR)

build/%.o: src/%.c $(SELF)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

b6xzp: $(ZPTOOL)
$(ZPTOOL): src/misc/b6xzp.c $(SELF)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

-include $(DEPS)

run: $(EMULATOR)
	$(EMULATOR) $(ROM)

clean:
	rm -rf build

version:
	@echo $(VERSION) 

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f $(EMULATOR) ${DESTDIR}${PREFIX}/bin
	cp -f $(ZPTOOL) ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/b6x
	chmod 755 ${DESTDIR}${PREFIX}/bin/b6xzp

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/b6x
	rm -f ${DESTDIR}${PREFIX}/bin/b6xzp

.PHONY: version clean install uninstall run b6xzp b6x all
