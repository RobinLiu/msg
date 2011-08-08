CXXFLAGS =	-O2 -g -Wall -fmessage-length=0

OBJS =	string_piece.o	Log.o lock_impl_posix.o logging.o safe_strerror_posix.o

LIBS = -lpthread -lrt

TARGET =	Log

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
