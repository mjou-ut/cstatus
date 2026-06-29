CC      = cc
CFLAGS  = -O2 -std=c99 -I.
TARGET  = cstatus
SRC     = cstatus.c random-metric.c

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

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(CJSON_CFLAGS) -o $@ $^ $(CJSON_LIBS)

clean:
	rm -f $(TARGET)
