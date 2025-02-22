#include <stacktrace>

int main()
{
  (void)to_string(std::stacktrace::current());
}