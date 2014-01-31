
#ifndef __ACTIONS_H__
#define __ACTIONS_H__

extern int command(const char *);
extern int focus(Client *, const char *);
extern int killclient(Client *);
extern int layout(const char *);
extern int mod_bar(const char *);
extern int mod_container(const char *);
extern int move(Client *, const char *);
extern int pull_client(Client *);
extern int push_client(Client *, Client *);
extern int spawn(const char *);
extern int tag(Client *, const char *);
extern int word(const char *);

#endif /* __ACTIONS_H__ */
