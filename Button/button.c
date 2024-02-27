#include <string.h>
#include "button.h"

void button_init(struct button* self) {
  memset(self, 0, sizeof(*self));
}

void button_deinit(struct button* self) {
  self->callback = NULL;
}

void button_add_callback(struct button* self, struct button_callback* cb) {
  self->callback = cb;
}

void button_remove_callback(struct button* self) {
  self->callback = NULL;
}

void button_do_somethind(struct button* self) {
  if (self->callback) {
    self->callback->callback(self->callback);  // call the callback
  }
}


void addTotal(struct button_callback* callback) {

}

//
int main()
{
  struct button btn;
  button_init(&btn);
  button_add_callback(&btn, addTotal);
  button_do_something(&btn);
  button_deinit(&btn);
}
