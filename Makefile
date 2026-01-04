
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I include
LDFLAGS = 

SRC_DIR = src/bolt
BUILD_DIR = build
TARGET = $(BUILD_DIR)/bolt

SRCS = $(shell find $(SRC_DIR) -name '*.cpp')
OBJS = $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)/*

# Terminal AI App
bolt_terminal_ai: bolt_terminal_ai.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) $< $(OBJS) -o $@ $(LDFLAGS)
