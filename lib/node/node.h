typedef struct _Node Node;
struct _Node {
	Node *head;
	void *data;
	Node *tail;
};

#define nd_foreach_child(i, node)\
	for((i) = node->tail;\
		(i) != NULL;\
		(i) = (i)->tail)

// Create a node with some data
Node *nd_make(void *data);
// Remove a node from a list, returning the node
Node *nd_rem(Node *nd);
// Remove a node from a list, freeing the node
Node *nd_free(Node *nd);
// Remove a node from a list, freeing it and and of its children
void nd_destroy(Node *nd);

// Get the last node
Node *nd_last(Node *nd);
// Get the first node
Node *nd_first(Node *nd);

// Append a child node, inserting if necessary
Node *nd_append(Node *nd, Node *new_child);
// Prepend a parent node, inserting if necessary
Node *nd_prepend(Node *nd, Node *new_head);

// Get the nth parent or child of a node, or the head or the tail if the offset is too large
Node *nd_get_nth(Node *nd, int index);

