#include "pqueue.h"

int
main(void)
	{
	pitem *item;
	pqueue pq;

	pq = pqueue_new();

	item = pitem_new(3, NULL);
	pqueue_insert(pq, item);

	item = pitem_new(1, NULL);
	pqueue_insert(pq, item);

	item = pitem_new(2, NULL);
	pqueue_insert(pq, item);

	item = pqueue_find(pq, 1);
	fprintf(stderr, "found %ld\n", item->priority);

	item = pqueue_find(pq, 2);
	fprintf(stderr, "found %ld\n", item->priority);

	item = pqueue_find(pq, 3);
	fprintf(stderr, "found %ld\n", item ? item->priority: 0);

	pqueue_print(pq);

	for(item = pqueue_pop(pq); item != NULL; item = pqueue_pop(pq))
		pitem_free(item);

	pqueue_free(pq);
	return 0;
	}
