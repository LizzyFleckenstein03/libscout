#include <stdlib.h>
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

scwaypoint *__scallocwayp(const scnode *node, scway *way)
{
	scwaypoint *wayp = malloc(sizeof(scwaypoint));
	wayp->nod = node;
	wayp->way = way;
	wayp->nxt = NULL;
	wayp->len = 0;
}

scwaypoint *__scstackfindgetend(scwaypoint *stack, scway *way)
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

int __scstackgetlen(scwaypoint *stack)
{
	for (scwaypoitn)
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
		
		if (wayp && wayp->len <= (tway->len = __scstackgetlen(tway)))
			__scstackfree(wayp);
		wayp = twayp;
	}
	return wayp;
}
