#include "opaque.h"

// actual definition of the struct in private space
struct opaque {
  int data;
};

int opaque_init(struct opaque* self) {
  memset(self, 0, sizeof(*self));  // zero all
  // do other initialization
  return 0;
}

int opaque_deinit(struct opaque* self) {
  // free any internal resources and return to known state
  self->data = 0;
  return 0;
}

void opaque_set_data(struct opaque* self, int val) {
  self->data = val;
}

int opaque_get_data(struct opaque* self) {
  return self->data;
}
