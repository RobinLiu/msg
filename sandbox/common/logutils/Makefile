CXXFLAGS =	-O2 -g -Wall -fmessage-length=0 -fPIC -DUSE_SYMBOLIZE 
#-DNDEBUG
LDFLAGS = -fPIC -shared
#CFLAGS= -fPIC -shared
OBJS =	string_piece.o	 lock_impl_posix.o logging.o safe_strerror_posix.o stack_trace_posix.o debugger_posix.o symbolize.o demangle.o

LIBS = -lpthread -lrt

TARGET =	liblog.so

$(TARGET):	$(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
