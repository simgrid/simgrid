#include <iostream>
#include <thread>
#include <vector>

// shared collection object
std::vector<int> v = {1, 2, 3, 5, 8, 13};

extern "C" {
extern int sthread_access_begin(void* addr, const char* objname, const char* file, int line, const char* func)
    __attribute__((weak));
extern void sthread_access_end(void* addr, const char* objname, const char* file, int line, const char* func)
    __attribute__((weak));
}

#define STHREAD_ACCESS(obj)                                                                                            \
  for (bool first = sthread_access_begin(static_cast<void*>(obj), #obj, __FILE__, __LINE__, __func__) || true; first;  \
       sthread_access_end(static_cast<void*>(obj), #obj, __FILE__, __LINE__, __func__), first = false)

static void thread_code()
{
  // Add another integer to the vector
  STHREAD_ACCESS(&v) v.push_back(21);
}

int main()
{
  std::cout << "starting two helpers...\n";
  std::thread helper1(thread_code);
  std::thread helper2(thread_code);

  std::cout << "waiting for helpers to finish..." << std::endl;
  helper1.join();
  helper2.join();

  // Print out the vector
  std::cout << "v = { ";
  for (int n : v)
    std::cout << n << ", ";
  std::cout << "}; \n";
}