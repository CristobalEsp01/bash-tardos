CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude

SRC_DIR = src
BIN_DIR = bin

TARGET_MAIN = $(BIN_DIR)/SistOpe
TARGET_MULTI = $(BIN_DIR)/multi

SOURCES_MAIN = $(SRC_DIR)/main.cpp $(SRC_DIR)/userModule.cpp $(SRC_DIR)/profileModule.cpp $(SRC_DIR)/palindromoModule.cpp $(SRC_DIR)/conteoTextoModule.cpp $(SRC_DIR)/conteoArchModule.cpp
SOURCES_MULTI = $(SRC_DIR)/matmul.cpp $(SRC_DIR)/matrixModule.cpp

OBJECTS_MAIN = $(SOURCES_MAIN:.cpp=.o)
OBJECTS_MULTI = $(SOURCES_MULTI:.cpp=.o)

RM_CMD = rm -f
MKDIR_CMD = mkdir -p

all: dirs $(TARGET_MAIN) $(TARGET_MULTI)

dirs:
	@$(MKDIR_CMD) $(BIN_DIR) 2>/dev/null || true

$(TARGET_MAIN): $(OBJECTS_MAIN)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS_MAIN)

$(TARGET_MULTI): $(OBJECTS_MULTI)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS_MULTI)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM_CMD) $(SRC_DIR)/*.o
	$(RM_CMD) $(BIN_DIR)/*
