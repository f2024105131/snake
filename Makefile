
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS += -Iinclude
LDFLAGS  ?=

SRC_DIR   := src
TEST_DIR  := tests
BUILD_DIR := build
BIN_DIR   := bin

SOURCES     := $(wildcard $(SRC_DIR)/*.cpp)
CORE_SRC    := $(filter-out $(SRC_DIR)/main.cpp,$(SOURCES))
OBJECTS     := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
CORE_OBJ    := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CORE_SRC))
TEST_SRC    := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJ    := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/tests_%.o,$(TEST_SRC))

TARGET      := $(BIN_DIR)/snake
TEST_TARGET := $(BIN_DIR)/snake_tests

PREFIX ?= /usr/local

.PHONY: all run test debug clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_TARGET): $(CORE_OBJ) $(TEST_OBJ) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/tests_%.o: $(TEST_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

run: $(TARGET)
	./$(TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

debug: CXXFLAGS := -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -fsanitize=address,undefined
debug: LDFLAGS  := -fsanitize=address,undefined
debug: clean $(TEST_TARGET)
	./$(TEST_TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/snake

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/snake

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)
