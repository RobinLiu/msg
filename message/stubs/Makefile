CFLAGS += -fmessage-length=0 \
          -I./ -I../../common/include/  -I../../common -I ../src
CXXFLAGS = 
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
OBJS += $(patsubst %.cpp,%.o,$(wildcard *.cpp))
LIBS =

TARGET = message_stub

$(TARGET):	$(OBJS)
	$(CC)  $(CXXFLAGS) $(OBJS) $(LIBS) -o $(TARGET)


all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

