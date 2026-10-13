/* struct and array walks with register variables */
struct node { int v; struct node *next; } nodes[40];
int tab[64];
int sum(register struct node *p) { register int s; s = 0; while (p) { s += p->v; p = p->next; } return s; }
int test(void)
{
	register int *q;
	register int i;
	int s, k;

	for (i = 0; i < 39; i++) { nodes[i].v = i; nodes[i].next = &nodes[i+1]; }
	nodes[39].v = 39; nodes[39].next = 0;
	for (i = 0; i < 64; i++) tab[i] = i;
	s = 0;
	for (k = 0; k < 6; k++) {
		s += sum(nodes);
		q = tab;
		for (i = 0; i < 64; i++)
			s += *q++;
	}
	return s;
}
