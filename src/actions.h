
#ifndef __ACTIONS_H__
#define __ACTIONS_H__

extern int command(const char *);
extern void conftile(Client *, const char **);
extern void float_full(Client *, const char **);
extern void focus(Client *, const char **);
extern void killclient(Client *, const char **);
extern void layout(Client *, const char **);
extern void mod_bar(Client *, const char **);
extern void move(Client *, const char **);
extern void pull_client(Client *);
extern void push_client(Client *, Client *);
extern void spawn(Client *, const char **);
extern void tag(Client *, const char **);
extern void word(const char *);

#endif /* __ACTIONS_H__ */
