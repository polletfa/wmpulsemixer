CFLAGS  += -D_DEFAULT_SOURCE $(shell pkg-config --cflags $(PKG_DEPS)) #-DDEBUG
LDFLAGS += -lpulse -lX11 -lXpm -lXext -lm
DESTDIR =
PREFIX = /usr/local

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

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

.PHONY: clean distclean install
