
// a structure containing some data
struct my_object {
  int variable;
  int flags;
};

int my_object_init(struct my_object *self);    // set up the object self
int my_object_deinit(struct my_object *self);  // tear down the object self
