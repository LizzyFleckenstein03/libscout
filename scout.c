#include <stdlib.h>
#include "scout.h"

scnode *scnodalloc(void *data)
{
	scnode *nod = malloc(sizeof(scnode));
	nod->way = NULL;
	nod->dat = data;
	return nod;
}

scway *scaddway(scnode *from, const scnode *to, float len, void *data)
{
	scway *way = malloc(sizeof(scway));
	way->lto = to;
	way->alt = NULL;
	way->len = len;
	way->dat = data;
	scway *par = __scnodgetway(from);
	if (par)
		par->alt = way;
	else
		from->way = way;
	return way;
}

int scisconnected(scnode *n1, scnode *n2) {
	for (scway *way = n1->way; way != NULL; way = way->alt) {
		if (way->lto == n2)
			return 1;
	}
	return 0;
}

scwaypoint *scout(const scnode *from, const scnode *to, scwaypoint *stack)
{
	scwaypoint *wayp = NULL;
	if (from == to)
		return __scallocwayp(from, NULL);
	scwaypoint *stackend = __scstackgetend(stack);
	for (scway *way = from->way; way != NULL; way = way->alt) {
		if (__scstackfind(stack, way))
			continue;
		scwaypoint *twayp = __scallocwayp(from, way);
		if (stack)
			stackend->nxt = twayp;
		if ((twayp->nxt = scout(way->lto, to, stack ? stack : twayp)))
			twayp->len += twayp->nxt->len;
		if (twayp->nxt && (! wayp || wayp->len > twayp->len)) { 
			scdestroypath(wayp);
			wayp = twayp;
		} else {
			scdestroypath(twayp);
		}
	}
	return stack ? (stackend->nxt = wayp) : wayp;
}

void scdestroypath(scwaypoint *stack)
{
	for (scwaypoint *sptr = stack; sptr != NULL; sptr = sptr->nxt)
		free(sptr);
}

scway *__scnodgetway(const scnode *node)
{
	scway *way;
	for (way = node->way; way != NULL && way->alt != NULL; way = way->alt);
	return way;
}

scwaypoint *__scallocwayp(const scnode *node, const scway *way)
{
	scwaypoint *wayp = malloc(sizeof(scwaypoint));
	wayp->nod = node;
	wayp->way = way;
	wayp->nxt = NULL;
	wayp->len = way ? way->len : 0.0f;
	return wayp;
}

int __scstackfind(const scwaypoint *stack, const scway *way)
{
	for (const scwaypoint *sptr = stack; sptr != NULL; sptr = sptr->nxt)
		if (sptr->nod == way->lto)
			return 1;
	return 0;
}

scwaypoint *__scstackgetend(scwaypoint *stack)
{
	scwaypoint *sptr;
	for (sptr = stack; sptr != NULL && sptr->nxt != NULL; sptr = sptr->nxt);
	return sptr;
}
