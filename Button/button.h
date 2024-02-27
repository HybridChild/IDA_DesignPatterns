#include <stddef.h>

struct button {
  struct button_callback* callback;
};

// this is a public callback definition
struct button_callback {
  void (*callback)(struct button_callback* callback);
};

void button_init(struct button* self);
void button_deinit(struct button* self);
void button_add_callback(struct button* self, struct button_callback* cb);
void button_remove_callback(struct button* self);
void button_do_something(struct button* self);
