SRC_DIR      = ../src
OBJ_DIR      = ../objs
TEST_OBJ_DIR = objs_tests

CXX	      = g++
CXXFLAGS  = -Wall -Wextra -Werror -std=c++17 -Wno-deprecated-declarations -I$(SRC_DIR) -MMD -MP -g3
GTEST_DIR = googletest/googletest
TEST_LIBS = -pthread
TEST_INCS = -I$(GTEST_DIR)/include -I$(GTEST_DIR)

TEST_BIN    = tests

TEST_SRCS   = test_main.cpp test_hashmap.cpp test_prefixtree.cpp
TEST_OBJS   = $(TEST_SRCS:%.cpp=$(TEST_OBJ_DIR)/%.o)

GTEST_OBJS     := $(TEST_OBJ_DIR)/gtest-all.o

TESTED_SRCS = $(SRC_DIR)/data_structures/PrefixTree.cpp
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