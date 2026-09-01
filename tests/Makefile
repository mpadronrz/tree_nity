SRC_DIR      = ../src
CLIENT_DIR   = ../src/client
CLIENT_INC_DIR = $(CLIENT_DIR)/inc
OBJ_DIR      = ../objs
TEST_OBJ_DIR = objs_tests

CXX	      = g++
CXXFLAGS  = -Wall -Wextra -Werror -std=c++17 -Wno-deprecated-declarations -I$(SRC_DIR) -I$(CLIENT_INC_DIR) -MMD -MP -g3
GTEST_DIR = googletest/googletest
TEST_LIBS = -pthread
TEST_INCS = -I$(GTEST_DIR)/include -I$(GTEST_DIR)

TEST_BIN    = tests


TEST_SRCS   = test_main.cpp test_hashmap.cpp test_prefixtree.cpp test_protocol.cpp test_main.cpp test_hashmap.cpp client/test_command.cpp client/test_message_parser.cpp \
	client/test_message_output.cpp client/test_exit_status.cpp client/test_fifo_io.cpp \
	client/test_shutdown_signal.cpp client/test_protocol_frame.cpp \
	client/test_protocol_payload.cpp client/test_request_builder.cpp \
	client/test_protocol_io.cpp client/test_ipc_exchange.cpp \
	client/test_management_command.cpp client/test_producer_command.cpp \
	client/test_subscription_io.cpp client/test_subscriber_command.cpp

TEST_OBJS   = $(TEST_SRCS:%.cpp=$(TEST_OBJ_DIR)/%.o)

GTEST_OBJS     := $(TEST_OBJ_DIR)/gtest-all.o

TESTED_SRCS = $(SRC_DIR)/client/src/command.cpp $(SRC_DIR)/client/src/message_parser.cpp \
	$(SRC_DIR)/client/src/message_output.cpp $(SRC_DIR)/client/src/exit_status.cpp \
	$(SRC_DIR)/client/src/fifo_io.cpp $(SRC_DIR)/client/src/shutdown_signal.cpp \
	$(SRC_DIR)/client/src/protocol_frame.cpp $(SRC_DIR)/client/src/protocol_payload.cpp \
	$(SRC_DIR)/client/src/request_builder.cpp $(SRC_DIR)/client/src/protocol_io.cpp \
	$(SRC_DIR)/client/src/ipc_exchange.cpp $(SRC_DIR)/client/src/management_command.cpp \
	$(SRC_DIR)/client/src/producer_command.cpp $(SRC_DIR)/client/src/subscription_io.cpp \
	$(SRC_DIR)/client/src/subscriber_command.cpp
TESTED_OBJS = $(TESTED_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)


OBJS = $(TEST_OBJS) $(TESTED_OBJS)
DEPS = $(OBJS:.o=.d)

all: $(TEST_BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(GTEST_OBJS) $(TEST_OBJS) $(TESTED_OBJS)
	$(CXX) $(CXXFLAGS) $(TEST_LIBS) $(TEST_INCS) $^ -o $@

$(TEST_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(TEST_LIBS) $(TEST_INCS) -c $< -o $@

$(TEST_OBJ_DIR)/gtest-all.o: $(GTEST_DIR)/src/gtest-all.cc
	@mkdir -p $(dir $@)
	$(CXX) -std=c++17 $(TEST_LIBS) $(TEST_INCS) -w -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(TEST_OBJ_DIR)

fclean: clean
	rm -rf $(TEST_BIN)

re: fclean all

.PHONY: all clean fclean re test
