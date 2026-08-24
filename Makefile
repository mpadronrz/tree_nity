SRC_DIR = src
OBJ_DIR = objs
BIN_DIR = bin

CXX	  = g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++17 -Wno-deprecated-declarations -I$(SRC_DIR) -MMD -MP -g3

SERVER_BIN = $(BIN_DIR)/server
CLIENT_BIN	= $(BIN_DIR)/client

SERVER_SRCS = $(SRC_DIR)/server/main.cpp

SERVER_OBJS = $(SERVER_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

CLIENT_SRCS = $(SRC_DIR)/client/main.cpp

CLIENT_OBJS = $(CLI_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

OBJS = $(SERVER_OBJS) $(CLIENT_OBJS)
DEPS = $(OBJS:.o=.d)

all: $(SERVER_BIN) $(CLIENT_BIN)

run-server: $(SERVER_BIN)
	./$(SERVER_BIN)

run-client: $(CLIENT_BIN)
	./$(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(SERVER_OBJS) -o $(SERVER_BIN)

$(CLIENT_BIN): $(CLIENT_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CLIENT_OBJS) -o $(CLIENT_BIN)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(BIN_DIR)

re: fclean all

.PHONY: all clean fclean re run-server run-client