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

PACKAGE = torghostng
VERSION = $(shell sed -n 's/.*#define VERSION "\([^"]*\)".*/\1/p' $(SRC_DIR)/torghostng.h)
ARCH = $(shell uname -m | sed -e 's/x86_64/amd64/' -e 's/^i[3-6]86$$/i386/' -e 's/aarch64/arm64/' -e 's/armv7l/armhf/')

BUILD_DIR = build
DEB_STAGE = $(BUILD_DIR)/deb-root
DEB_OUT = $(BUILD_DIR)/$(PACKAGE)_$(VERSION)_$(ARCH).deb
RPM_TOPDIR = $(CURDIR)/$(BUILD_DIR)/rpm
DIST_DIR = $(BUILD_DIR)/dist

.PHONY: all clean install uninstall test memcheck deb rpm dist

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

deb: $(TARGET)
	rm -rf $(DEB_STAGE)
	mkdir -p $(DEB_STAGE)/DEBIAN
	mkdir -p $(DEB_STAGE)/usr/bin
	mkdir -p $(DEB_STAGE)/usr/share/man/man1
	install -m 0755 $(TARGET) $(DEB_STAGE)/usr/bin/$(TARGET)
	install -m 0644 packaging/man/torghostng.1 \
		$(DEB_STAGE)/usr/share/man/man1/$(TARGET).1
	sed -e 's/@VERSION@/$(VERSION)/g' -e 's/@ARCH@/$(ARCH)/g' \
		packaging/deb/control.in > $(DEB_STAGE)/DEBIAN/control
	packaging/scripts/build-deb.sh $(DEB_STAGE) $(DEB_OUT)

dist:
	rm -rf $(DIST_DIR) $(BUILD_DIR)/$(PACKAGE)-$(VERSION).tar.gz
	mkdir -p $(DIST_DIR)/$(PACKAGE)-$(VERSION)
	cp -a Makefile install.sh shell.nix LICENSE README.md src tests packaging \
		$(DIST_DIR)/$(PACKAGE)-$(VERSION)/
	rm -f $(DIST_DIR)/$(PACKAGE)-$(VERSION)/src/*.o \
		$(DIST_DIR)/$(PACKAGE)-$(VERSION)/src/*.d
	cd $(DIST_DIR) && tar -czf ../$(PACKAGE)-$(VERSION).tar.gz $(PACKAGE)-$(VERSION)

rpm: dist
	mkdir -p $(RPM_TOPDIR)/BUILD $(RPM_TOPDIR)/BUILDROOT \
		$(RPM_TOPDIR)/RPMS $(RPM_TOPDIR)/SOURCES \
		$(RPM_TOPDIR)/SPECS $(RPM_TOPDIR)/SRPMS
	cp $(BUILD_DIR)/$(PACKAGE)-$(VERSION).tar.gz $(RPM_TOPDIR)/SOURCES/
	rpmbuild --nodeps --define "_topdir $(RPM_TOPDIR)" -bb packaging/$(PACKAGE).spec

clean:
	rm -rf $(TARGET) $(OBJS) $(DEPS) $(BUILD_DIR)

-include $(DEPS)
