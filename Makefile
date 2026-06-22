CXX = g++
CXXFLAGS = -std=c++98 -Wall -Wextra -I./include
SRCDIR = src
INCDIR = include
BINDIR = bin

# Source files
SOURCES = main.cpp $(SRCDIR)/server.cpp $(SRCDIR)/logger.cpp $(SRCDIR)/Client.cpp 
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = $(BINDIR)/webserver

# Default target
all: $(TARGET)

# Create bin directory if it doesn't exist
$(BINDIR):
	mkdir -p $(BINDIR)

# Link target
$(TARGET): $(BINDIR) $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS)
	@echo "Build successful! Run with: ./$(TARGET)"

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS)
	rm -rf $(BINDIR)

# Run the server
run: $(TARGET)
	./$(TARGET)

# Rebuild
rebuild: clean all

.PHONY: all clean run rebuild
