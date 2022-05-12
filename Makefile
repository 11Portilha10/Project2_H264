# CC = g++
# CPPFLAGS = -g -I. -Wall
# DEPS = block.h frame_encode.h frame.h intra.h macroblock.h qdct.h
# OBJS = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
# TARGET = test

# %.o: %.cpp  $(DEPS)
# 	$(CC) -c -o $@ $< $(CPPFLAGS)

# $(TARGET): $(OBJS)
# 	$(CC) -o $(TARGET) $(OBJS) -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -lopencv_highgui

# clean:
# 	rm -f *.o $(TARGET)

#################################################################################

CC = g++

SRC_DIR = ./src
OBJ_DIR = ./obj
BIN_DIR = ./bin
INC_DIR = ./inc

TARGET = $(BIN_DIR)/test

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.cpp=.o)))

LIBS =
INCLUDE = -I$(INC_DIR)
CPPFLAGS += -Wall -g
LDFLAGS +=
LDLIBS += -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -lopencv_highgui

.PHONY: all clean

all: clean $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<

clean:
	$(RM) $(OBJS)

run:
	$(TARGET)