#ifndef __LIBSCOUT__
#define __LIBSCOUT__

struct scnode {
	struct scway *way;
	void *dat;
};

struct scway {
	const struct scnode *lto;
	struct scway *alt;
	int len;
};

struct scwaypoint {
	const struct scnode *nod;
	const struct scway *way;
	struct scwaypoint *nxt;
	int len;
};

struct scway *scaddway(struct scnode *, const struct scnode *, int);
struct scwaypoint *scout(const struct scnode *, const struct scnode *, struct scwaypoint *);

#ifdef __LIBSCOUT_INTERNAL__

struct scwaypoint *__scallocwayp(const struct scnode *, const struct scway *);
struct scwaypoint *__scstackfindgetend(struct scwaypoint *, const struct scway *);
void __scstackfree(struct scwaypoint *);
int __scstackgetlen(struct scwaypoint *);

#endif // __LIBSCOUT_INTERNAL__

#endif // __LIBSCOUT__
