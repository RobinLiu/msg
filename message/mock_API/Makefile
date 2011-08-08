CFLAGS +=	-fmessage-length=0 -fPIC -shared \
          -I./ -I../.. -I ../src
CXXFLAGS = -fPIC -shared
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
OBJS += $(patsubst %.cpp,%.o,$(wildcard *.cpp))
LIBS =

TARGET = ../../libs/libmock.so

$(TARGET):	$(OBJS)
	$(CC)  $(CXXFLAGS) $(OBJS) $(LIBS) -o $(TARGET)


all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

