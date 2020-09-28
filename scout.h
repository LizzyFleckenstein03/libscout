#ifndef _LIBSCOUT_H_
#define _LIBSCOUT_H_

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

#endif
