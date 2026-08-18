CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pthread

SERVER = $(HOME)/LJS_server/LJS
CLIENT = client

SERVER_SRC = src/LJS_server.cpp \
             src/isolate_utils.cpp \
             src/process_utils.cpp \
             src/judge.cpp

CLIENT_SRC = src/LJS_client.cpp

SERVER_OBJ = $(SERVER_SRC:.cpp=.o)
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(CLIENT): $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(SERVER) $(CLIENT)

.PHONY: all clean
