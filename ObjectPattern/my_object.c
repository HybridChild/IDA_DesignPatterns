
#include "my_object.h"

// a structure holding an object of struct my_object
struct application {
  struct my_object obj;
};

// application_init sets up its my_object member via my_object_init
int application_init(struct application *self) {
  return my_object_init(&self->obj);
}
