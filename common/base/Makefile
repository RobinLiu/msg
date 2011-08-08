CFLAGS +=	-fmessage-length=0 -fPIC -shared -I./ -I../../
CXXFLAGS += -fPIC -shared
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
OBJS += $(patsubst %.cpp,%.o,$(wildcard *.cpp))
LIBS =

TARGET = ../../libs/libbase.so

$(TARGET):	$(OBJS)
	$(CC)  $(CXXFLAGS) $(OBJS) $(LIBS) -o $(TARGET)


all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

