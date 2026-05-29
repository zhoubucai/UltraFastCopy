CXX:=g++
CXXFLAGS:=-std=c++17 -O2 -Wall
SRCS:=src/main.cpp src/UltraFastCopy.cpp src/Utils.cpp src/Logger.cpp
TARGET:=bin/ufcp
PREFIX:=/usr/local

.PHONY:all clean run install uninstall

all:$(TARGET)

$(TARGET):$(SRCS)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) -pthread

run:$(TARGET)
	./$(TARGET)

clean:
	rm -rf bin

install:$(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/ufcp

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/ufcp
