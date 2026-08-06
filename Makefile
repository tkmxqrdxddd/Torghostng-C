CC ?= gcc
PKG_CONFIG ?= pkg-config

CURL_CFLAGS := $(shell $(PKG_CONFIG) --cflags libcurl 2>/dev/null)
CURL_LIBS := $(shell $(PKG_CONFIG) --libs libcurl 2>/dev/null)
ifeq ($(strip $(CURL_LIBS)),)
CURL_LIBS = -lcurl
endif

CFLAGS ?= -O2
CFLAGS += -std=c99 -Wall -Wextra -pedantic -MMD -MP
CPPFLAGS += $(CURL_CFLAGS)
LDLIBS += $(CURL_LIBS)

TARGET = torghostng
SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
DESTDIR ?=

.PHONY: all clean install uninstall test memcheck

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDLIBS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

test: $(TARGET)
	./tests/cli_tests.sh

memcheck: $(TARGET)
	valgrind --leak-check=full --error-exitcode=1 ./$(TARGET) --version

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)
