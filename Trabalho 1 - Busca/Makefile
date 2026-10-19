CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Werror -O2 -D_POSIX_C_SOURCE=200809L -MMD -MP
TARGET  := trabalho1
BUILD   := build

SOURCES := $(wildcard src/*.c)
OBJECTS := $(patsubst src/%.c,$(BUILD)/%.o,$(SOURCES))

# SEED e ARGS permitem ajustar a execução: make run SEED=42 ARGS=--no-anim
RUN_ARGS := $(if $(SEED),--seed $(SEED)) $(ARGS)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $(BUILD)

run: all
	./$(TARGET) $(RUN_ARGS)

clean:
	rm -rf $(BUILD) $(TARGET)

-include $(OBJECTS:.o=.d)
