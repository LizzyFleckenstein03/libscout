#ifndef __LIBSCOUT__
#define __LIBSCOUT__

struct scnode {
	struct scway *way;
	void *dat;
};

struct scway {
	const struct scnode *lto;
	struct scway *alt;
	float len;
	void *dat;
};

struct scwaypoint {
	const struct scnode *nod;
	const struct scway *way;
	struct scwaypoint *nxt;
	float len;
};

struct scnode *scnodalloc(void *);
struct scway *scaddway(struct scnode *, const struct scnode *, float, void *data);
int scisconnected(struct scnode *, struct scnode *);
struct scwaypoint *scout(const struct scnode *, const struct scnode *, struct scwaypoint *);
void scdestroypath(struct scwaypoint *);


#ifdef __LIBSCOUT_INTERNAL__

struct scway *__scnodgetway(const struct scnode *);
struct scwaypoint *__scallocwayp(const struct scnode *, const struct scway *);
int __scstackfind(const struct scwaypoint *, const struct scway *);
struct scwaypoint *__scstackgetend(struct scwaypoint *);

#endif // __LIBSCOUT_INTERNAL__

#ifdef __LIBSCOUT_TYPEDEF__

typedef struct scnode scnode;
typedef struct scway scway;
typedef struct scwaypoint scwaypoint;

#endif

#endif // __LIBSCOUT__
