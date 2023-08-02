
#include <cstdio>
#include <cstdlib>

class Message
{
public:
  Message(const char *fname, int line)  : fname_(fname), line_(line) 
  {
    printf("Class created.\n");
  }
  ~Message()
  {
    printf("Message class destructed.\n");
  }

  template<typename... Args>
  void write(Args&&... args) {
    int nargs = sizeof...(args);
    printf("fname %s: line %d, # args %d\n", fname_, line_, nargs);
    // fixme: Fully implement `write` function.
  }

private:
  Message() = delete;
  Message(const Message&) = delete;

  const char *const fname_;
  const int line_;
};

#define msg(...) Message(__FILE__, __LINE__).write(__VA_ARGS__);

int main(int argc, char **argv)
{
        msg();
        msg("bora");
}
