SRC_DIR      = src
CLIENT_DIR   = $(SRC_DIR)/client
CLIENT_INC_DIR = $(CLIENT_DIR)/inc
SERVER_DIR   = $(SRC_DIR)/server
SERVER_INC_DIR = $(SERVER_DIR)/inc
OBJ_DIR      = objs
TEST_DIR     = tests

CXX	      = g++
CXXFLAGS  = -Wall -Wextra -Werror -std=c++17 -Wno-deprecated-declarations -I$(SRC_DIR) -I$(SERVER_INC_DIR) -I$(CLIENT_INC_DIR) -MMD -MP -g3

SERVER_BIN  = server
CLIENT_BIN	= client

SERVER_SRCS = $(SRC_DIR)/main.cpp $(SERVER_DIR)/src/Server.cpp \
	$(SERVER_DIR)/src/Topic.cpp $(SRC_DIR)/protocol/Protocol.cpp \
	$(SRC_DIR)/data_structures/PrefixTree.cpp $(SRC_DIR)/ipc/ipc.cpp
SERVER_OBJS = $(SERVER_SRCS:%.cpp=$(OBJ_DIR)/%.o)

CLIENT_SRCS = $(CLIENT_DIR)/src/main.cpp $(CLIENT_DIR)/src/command.cpp \
	$(CLIENT_DIR)/src/message_output.cpp \
	$(CLIENT_DIR)/src/exit_status.cpp \
	$(CLIENT_DIR)/src/shutdown_signal.cpp \
	$(CLIENT_DIR)/src/ipc_exchange.cpp \
	$(CLIENT_DIR)/src/management_command.cpp $(CLIENT_DIR)/src/producer_command.cpp \
	$(CLIENT_DIR)/src/subscriber_command.cpp \
	$(SRC_DIR)/protocol/Protocol.cpp $(SRC_DIR)/ipc/ipc.cpp
CLIENT_OBJS = $(CLIENT_SRCS:%.cpp=$(OBJ_DIR)/%.o)

OBJS = $(SERVER_OBJS) $(CLIENT_OBJS)
DEPS = $(OBJS:.o=.d)

all: $(SERVER_BIN) $(CLIENT_BIN)

run-server: $(SERVER_BIN)
	./server

run-client: $(CLIENT_BIN)
	./client

$(SERVER_BIN): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) $(SERVER_OBJS) -o $(SERVER_BIN)

$(CLIENT_BIN): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $(CLIENT_OBJS) -o $(CLIENT_BIN)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	@$(MAKE) -C $(TEST_DIR) clean
	rm -rf $(OBJ_DIR) $(TEST_OBJ_DIR)

fclean: clean
	@$(MAKE) -C $(TEST_DIR) fclean
	rm -rf server client

re: fclean all

test:
	@$(MAKE) -C $(TEST_DIR) test

.PHONY: all clean fclean re run-server run-client test
