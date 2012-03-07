CFLAGS += -I./ -I../../common/include/  -I../../common -I ../../
CXXFLAGS = 
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
OBJS += $(patsubst %.cpp,%.o,$(wildcard *.cpp))
LIBS = -lpthread  -L../../libs -llogging  -lbase -lclient -lbase -lrt

TARGET = testapp

$(TARGET):	$(OBJS)
	$(CC)  $(CFLAGS) $(OBJS) $(LIBS) -o $(TARGET)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

