#include <sstream>
#include <iostream>

class LogMessageVoidify {
 public:
  LogMessageVoidify() { }
  // This has to be an operator with a precedence lower than << but
  // higher than ?:
  void operator&(std::ostream&) {std::cout<<"called"<<std::endl; }
};

int main(void) {
    std::ostringstream stream_;
    LogMessageVoidify() & (stream_) <<"This is a test\n";
    //std::cout.write(stream_.str(), sizeof(stream_.str()));   
    std::cout<<stream_.str();
}
