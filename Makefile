CC      = cc

CFLAGS  = -Wall -Wextra -Wpedantic -Wno-format-truncation -std=c11 -O2 -D_GNU_SOURCE
LDFLAGS =

TARGET  = target/riftnest

SRC     = $(shell find src -type f -name '*.c' ! -path '*/vendor/*')
OBJ     = $(patsubst src/%.c,target/obj/%.o,$(SRC))

PROOT_BIN    = target/proot
PROOT_VENDOR = src/backends/proot/vendor/proot
PROOT_URL    = https://proot.gitlab.io/proot/bin/proot

.PHONY: all clean proot

all: proot $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

target/obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

proot:
	@if [ ! -x "$(PROOT_BIN)" ]; then \
		echo "Downloading proot static binary..."; \
		mkdir -p target; \
		curl -sL "$(PROOT_URL)" -o "$(PROOT_BIN)"; \
		chmod +x "$(PROOT_BIN)"; \
		echo "proot installed at $(PROOT_BIN)"; \
	fi

clean:
	rm -rf target
