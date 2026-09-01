SRC_DIR      = src
CLIENT_DIR   = client
CLIENT_INC_DIR = $(CLIENT_DIR)/inc
OBJ_DIR      = objs
BIN_DIR      = bin
TEST_DIR     = tests

CXX	      = g++
CXXFLAGS  = -Wall -Wextra -Werror -std=c++17 -Wno-deprecated-declarations -I$(SRC_DIR) -I$(CLIENT_INC_DIR) -MMD -MP -g3

SERVER_BIN  = $(BIN_DIR)/server
CLIENT_BIN	= $(BIN_DIR)/client

SERVER_SRCS = $(SRC_DIR)/server/main.cpp
SERVER_OBJS = $(SERVER_SRCS:%.cpp=$(OBJ_DIR)/%.o)

CLIENT_SRCS = $(CLIENT_DIR)/src/main.cpp $(CLIENT_DIR)/src/command.cpp \
	$(CLIENT_DIR)/src/message_parser.cpp $(CLIENT_DIR)/src/message_output.cpp \
	$(CLIENT_DIR)/src/exit_status.cpp $(CLIENT_DIR)/src/fifo_io.cpp \
	$(CLIENT_DIR)/src/shutdown_signal.cpp $(CLIENT_DIR)/src/protocol_frame.cpp \
	$(CLIENT_DIR)/src/protocol_payload.cpp $(CLIENT_DIR)/src/request_builder.cpp \
	$(CLIENT_DIR)/src/protocol_io.cpp $(CLIENT_DIR)/src/ipc_exchange.cpp \
	$(CLIENT_DIR)/src/management_command.cpp
CLIENT_OBJS = $(CLIENT_SRCS:%.cpp=$(OBJ_DIR)/%.o)

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

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	@$(MAKE) -C $(TEST_DIR) clean
	rm -rf $(OBJ_DIR) $(TEST_OBJ_DIR)

fclean: clean
	@$(MAKE) -C $(TEST_DIR) fclean
	rm -rf $(BIN_DIR)

re: fclean all

test:
	@$(MAKE) -C $(TEST_DIR) test

.PHONY: all clean fclean re run-server run-client test
