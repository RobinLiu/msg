CFLAGS +=	-fmessage-length=0  -I./ -I../../ #-fPIC -shared
#CXXFLAGS += -fPIC -shared
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
OBJS += $(patsubst %.cpp,%.o,$(wildcard *.cpp))
LIBS =

TARGET = ../../libs/libbase.a

$(TARGET):	$(OBJS)
#	$(CC)  $(CXXFLAGS) $(OBJS) $(LIBS) -o $(TARGET)
	ar rcs $(TARGET) $(OBJS)


all:	$(TARGET)
clean:
	rm -f $(OBJS) $(TARGET)

