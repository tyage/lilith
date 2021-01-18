include ../Makefile.common

SRCS := main.cpp value.cpp prelude.cpp allocator.cpp lisp_prelude.cpp
OBJS := $(SRCS:%.cpp=%.o)
DEPS := $(SRCS:%.cpp=%.d)

CXXFLAGS := -Wall -Wextra -std=c++20
-include $(DEPS)

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -MMD -MP $<

debug: CXXFLAGS += -DDEBUG -g
debug: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJS) $(DEPS)
