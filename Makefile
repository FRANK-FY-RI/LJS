CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread

SERVER = $(HOME)/LJS_server/LJS
CLIENT = client

SERVER_SRC = src/LJS_server.cpp \
             src/isolate_utils.cpp \
             src/process_utils.cpp \
             src/judge.cpp

CLIENT_SRC = src/LJS_client.cpp

SQLITE_SRC = third_party/sqlite3/sqlite3.c

SERVER_OBJ = $(SERVER_SRC:.cpp=.o)
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)
SQLITE_OBJ = $(SQLITE_SRC:.c=.o)

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_OBJ) $(SQLITE_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(CLIENT): $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -Ithird_party/sqlite -c $< -o $@

%.o: %.c
	$(CC) -Ithird_party/sqlite -c $< -o $@

clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(SQLITE_OBJ) $(SERVER) $(CLIENT)

.PHONY: all clean
