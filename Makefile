CC      = cc

CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -O2

TARGET  = target/riftnest

SRC     = $(shell find src -type f -name '*.c')
OBJ     = $(patsubst src/%.c,target/obj/%.o,$(SRC))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

target/obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf target