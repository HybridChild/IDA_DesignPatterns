
struct opaque; // forward declaration

// methods that operate on an opaque object
int opaque_init(struct opaque *self);
int opaque_deinit(struct opaque *self);
void opaque_set_data(struct opaque *self, int data);
int opaque_get_data(struct opaque *self);
