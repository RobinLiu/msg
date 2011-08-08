CXXFLAGS =	-O2 -g -Wall -fmessage-length=0 -I./ 

OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
OBJS += $(patsubst %.cpp,%.o,$(wildcard *.cpp))
LIBS = -lpthread  -lrt -L ../libs -lglog

TARGET = mutex

$(TARGET):	$(OBJS)
	$(CXX)  $(CXXFLAGS) $(OBJS) $(LIBS) -o $(TARGET)


all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

