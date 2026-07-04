#Makefile to run Stress Tester

CXX      := g++
CXXFLAGS := -std=c++17 -Wreorder -Wall -Wextra -O1 -fsanitize=thread -g 

# Target executable name
TARGET   := stress_tester


.PHONY: all clean test
.ONESHELL:

# 1. Compile the binary
all: $(TARGET)

$(TARGET): Stress_Test.cpp task.h graph.h scheduler.h logger.h
	@echo "Compiling $(TARGET)..."
	$(CXX) $(CXXFLAGS) Stress_Test.cpp -o $(TARGET) 
	@echo "Done!"

# 2. Automatically loop through and run every test file found
test: $(TARGET)
	./$(TARGET)

# 3. Clean up binary files
clean:
	rm -f $(TARGET)