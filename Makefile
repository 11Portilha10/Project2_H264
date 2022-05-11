CC = g++
CPPFLAGS = -g -I. -Wall
DEPS = block.h frame_encode.h frame.h intra.h macroblock.h qdct.h
OBJS = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
TARGET = test

%.o: %.cpp  $(DEPS)
	$(CC) -c -o $@ $< $(CPPFLAGS)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -lopencv_highgui

clean:
	rm -f *.o $(TARGET)