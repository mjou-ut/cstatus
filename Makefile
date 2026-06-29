CC      = cc
CFLAGS  = -O2 -std=c99 -I.
TARGET  = cstatus
SRC     = cstatus.c random-metric.c
VERSION := $(shell cat VERSION 2>/dev/null || echo "0.0.0")
CFLAGS += -DVERSION=\"$(VERSION)\"

# Locate cJSON via pkg-config, then Homebrew, then bare -lcjson
CJSON_CFLAGS := $(shell pkg-config --cflags cjson 2>/dev/null)
CJSON_LIBS   := $(shell pkg-config --libs   cjson 2>/dev/null)
ifeq ($(CJSON_CFLAGS),)
  BREW_CJSON   := $(shell brew --prefix cjson 2>/dev/null)
  ifneq ($(BREW_CJSON),)
    CJSON_CFLAGS := -I$(BREW_CJSON)/include
    CJSON_LIBS   := -L$(BREW_CJSON)/lib -lcjson
  else
    CJSON_LIBS   := -lcjson
  endif
endif

.PHONY: all clean bump

all: $(TARGET)

$(TARGET): $(SRC) VERSION
	$(CC) $(CFLAGS) $(CJSON_CFLAGS) -o $@ $(SRC) $(CJSON_LIBS)

clean:
	rm -f $(TARGET)

# bump – increment the minor version (0.x.y → 0.(x+1).0)
bump:
	@OLD=$$(cat VERSION | tr -d '\n'); \
	OLD_MINOR=$$(echo $$OLD | cut -d. -f2); \
	NEW_MINOR=$$(( $$OLD_MINOR + 1 )); \
	NEW_VERSION=$$(echo $$OLD | cut -d. -f1).$$NEW_MINOR.0; \
	echo "bumped $$OLD → $$NEW_VERSION"; \
	printf '%s\n' "$$NEW_VERSION" > VERSION; \
	$(MAKE)
