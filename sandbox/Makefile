CXXFLAGS =	-O2 -g -Wall -fmessage-length=0 -I../logutils

OBJS =	main.o Mutex.o MutexHolder.o message_center.o

LIBS = -lpthread -lrt

TARGET =	logtest

$(TARGET):	$(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
