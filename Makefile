CXX:=g++
SRCS:=src/main.cpp src/UltraFastCopy.cpp src/Utils.cpp src/Logger.cpp
TARGET:=bin/copy

.PHONY:clean

all:$(TARGET)

$(TARGET):$(SRCS)
	mkdir -p bin
	$(CXX) $(SRCS) -o $(TARGET)

run:$(TARGET)
	./$(TARGET)

clean:
	rm -rf $(TARGET)

