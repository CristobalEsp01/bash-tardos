CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude
# libstdc++ estatica: el ejecutable funciona en otros Linux aunque tengan otra version de g++
LDFLAGS  = -static-libstdc++ -static-libgcc

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

TARGET_MAIN  = $(BIN_DIR)/SistOpe
TARGET_ADMIN = $(BIN_DIR)/admin
TARGET_MULTI = $(BIN_DIR)/multi

COMMON  = config.cpp userModule.cpp profileModule.cpp
SRC_MAIN  = main.cpp palindromoModule.cpp conteoTextoModule.cpp conteoArchModule.cpp $(COMMON)
SRC_ADMIN = admin.cpp $(COMMON)
SRC_MULTI = matmul.cpp

OBJ_MAIN  = $(addprefix $(OBJ_DIR)/,$(SRC_MAIN:.cpp=.o))
OBJ_ADMIN = $(addprefix $(OBJ_DIR)/,$(SRC_ADMIN:.cpp=.o))
OBJ_MULTI = $(addprefix $(OBJ_DIR)/,$(SRC_MULTI:.cpp=.o))

.PHONY: all clean run

all: $(TARGET_MAIN) $(TARGET_ADMIN) $(TARGET_MULTI)

$(TARGET_MAIN): $(OBJ_MAIN) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_ADMIN): $(OBJ_ADMIN) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_MULTI): $(OBJ_MULTI) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(wildcard include/*.h) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

# Ejemplo: make run ARGS='-u MaxAR -p 1001 -f data/LIBROS/drama/hamlet_shakespeare.txt'
run: all
	./$(TARGET_MAIN) $(ARGS)

clean:
	rm -rf $(OBJ_DIR) $(TARGET_MAIN) $(TARGET_ADMIN) $(TARGET_MULTI)
