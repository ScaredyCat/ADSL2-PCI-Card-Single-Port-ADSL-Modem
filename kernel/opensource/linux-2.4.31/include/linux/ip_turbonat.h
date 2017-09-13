typedef int (*turbonat_hook_cb)(unsigned int,struct sk_buff **);
typedef int (*addroute_hook_cb)(struct sk_buff *);
extern turbonat_hook_cb turbonat_hook_fn ;
extern addroute_hook_cb addroute_hook_fn ;
