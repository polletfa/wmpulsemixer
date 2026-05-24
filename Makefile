CFLAGS  += -D_DEFAULT_SOURCE $(shell pkg-config --cflags $(PKG_DEPS)) #-DDEBUG
LDFLAGS += -lpulse -lX11 -lXpm -lXext -lm
PREFIX = /usr/bin/

CC = cc
OBJS = wmpulsemixer.o pulse.o
TARGET = wmpulsemixer

XPMS = XPM/chars.xpm \
       XPM/digits.xpm \
       XPM/icons.xpm \
       XPM/muted.xpm \
       XPM/tile.xpm \
       XPM/wmpulsemixer.xpm

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

pulse.o: pulse.c pulse.h
wmpulsemixer.o: wmpulsemixer.c $(XPMS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

distclean: clean
	rm -f $(TARGET)

install:
	mkdir -p $(PREFIX)/ && install -m 755 $(TARGET) $(PREFIX)/

.PHONY: clean distclean install
