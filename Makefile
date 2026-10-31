VERSION = 0.2.1

PKG_DEPS = x11 xext xpm libpulse libpulse-mainloop-glib gtk+-3.0
CFLAGS  += -DVERSION=\"$(VERSION)\" -D_DEFAULT_SOURCE $(shell pkg-config --cflags $(PKG_DEPS))
LDFLAGS += -lm $(shell pkg-config --libs $(PKG_DEPS))
DESTDIR =
PREFIX = /usr/local
DEPFLAGS = -MMD -MP

CC = cc
SRCS = wmpulsemixer.c pulse.c menu.c window.c options.c ui.c
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
TARGET = wmpulsemixer

XPMS = XPM/chars.xpm \
       XPM/digits.xpm \
       XPM/icons.xpm \
       XPM/muted.xpm \
       XPM/tile.xpm \
       XPM/wmpulsemixer.xpm

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) $(DEPS) *.bak

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

.PHONY: clean install

-include $(DEPS)
