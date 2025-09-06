CXX = g++
CFLAGS = -std=c++11 -Wall -Wextra -pthread
LDFLAGS = -pthread

SRCS = main.cpp epoll_server.cpp tlv_protocol.cpp message_queue.cpp byte_converter.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = epoll_server

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)