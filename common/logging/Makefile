CFLAGS +=	-fmessage-length=0  -I./ -I../../ #-fPIC -shared
#CXXFLAGS += -fPIC -shared
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
OBJS += $(patsubst %.cpp,%.o,$(wildcard *.cpp))
LIBS =

TARGET = ../../libs/liblogging.a

$(TARGET):	$(OBJS)
#	$(CC)  $(CFLAGS) $(OBJS) $(LIBS)
	ar rcs $(TARGET) $(OBJS)


all:	$(TARGET)
clean:
	rm -f $(OBJS) $(TARGET)

