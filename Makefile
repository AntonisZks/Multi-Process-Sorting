# Compliler and flags compilation
CC = g++
FLAGS = -g -Wall

# Constants for build, source and bin directories
SRC_DIR = src
OBJ_DIR = build
EXE_DIR = bin

# Compilation command
all: build bin $(EXE_DIR)/mysort

$(EXE_DIR)/mysort: $(OBJ_DIR)/main.o
	$(CC) $(FLAGS) -o $(EXE_DIR)/mysort $(OBJ_DIR)/main.o

$(OBJ_DIR)/main.o: $(SRC_DIR)/main.cpp
	$(CC) $(FLAGS) -o $(OBJ_DIR)/main.o -c $(SRC_DIR)/main.cpp

.PHONY: clean

# Create the build directory for the object files
build:
	mkdir build

# Create the bin directory for the executable files
bin:
	mkdir bin

# Commands that cleans the workspace
clean:
	rm $(OBJ_DIR)/main.o $(EXE_DIR)/mysort
	rmdir build
	rmdir bin
