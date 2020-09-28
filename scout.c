#include <stdlib.h>
#define __LIBSCOUT_INTERNAL__
#include "scout.h"

typedef struct scnode scnode;
typedef struct scway scway;
typedef struct scwaypoint scwaypoint;

scway *scaddway(scnode *from, const scnode *to, int len)
{
	scway *way = malloc(sizeof(scway));
	way->lto = to;
	way->alt = NULL;
	way->len = len;
	scway *apar, *par = NULL;
	for (apar = from->way; apar != NULL; par = apar, apar = apar->alt);
	if (par)
		par->alt = way;
	else
		from->way = way;
	return way;
}

scwaypoint *scout(const scnode *from, const scnode *to, scwaypoint *stack)
{
	scwaypoint *wayp = NULL;
	if (from == to)
		return __scallocwayp(from, NULL);
	for (scway *way = from->way; way != NULL; way = way->alt) {
		scwaypoint *stackend;
		if ((stackend = __scstackfindgetend(stack, way)) == NULL)
			continue;
		scwaypoint *twayp = __scallocwayp(from, way);
		stackend->nxt = twayp;
		scwaypoint *nwayp = scout(way->lto, to, stack)
		if (wayp && wayp->len <= (twayp->len = __scstackgetlen(twayp)))
			__scstackfree(wayp);
		wayp = twayp;
	}
	return wayp;
}

scwaypoint *__scallocwayp(const scnode *node, const scway *way)
{
	scwaypoint *wayp = malloc(sizeof(scwaypoint));
	wayp->nod = node;
	wayp->way = way;
	wayp->nxt = NULL;
	wayp->len = way->len;
}

scwaypoint *__scstackfindgetend(scwaypoint *stack, const scway *way)
{
	scwaypoint *asptr, *sptr;
	for (asptr = stack; asptr != NULL; sptr = asptr, asptr = asptr->nxt)
		if (asptr->nod == way->lto)
			return NULL;
	return sptr;
}

void __scstackfree(scwaypoint *stack)
{
	for (scwaypoint *sptr = stack; sptr != NULL; sptr = sptr->nxt)
		free(sptr);
}
