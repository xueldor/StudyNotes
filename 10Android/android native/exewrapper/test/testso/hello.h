#ifndef HELLO_H
#define HELLO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LOG_TAG
#define LOG_TAG "Hello"
#endif

namespace hello {

class World {
 private:

 public:
   World()=default;
  ~World()=default;
  void say();
};
}  // namespace hello

#endif  // HELLO_H
