#ifndef __MEMORY_TRACE
#define __MEMORY_TRACE

#define printf(fmt, args...) syslog(LOG_ERR, fmt, ##args)

#define calloc(n,s) ({ void *_ptr;                      \
                   _ptr = calloc(n,s);                          \
               printf(" calloc ptr/size <0x%x: %d bytes> from <%s: line %d> !\n",_ptr,s*n,__FILE__,__LINE__); \
                   _ptr; })


#define malloc(s) calloc(1,s)



#define realloc(old_ptr,s) ({ void *new_ptr;         \
          printf("realloc::free ptr <0x%x:> from <%s: line %d> !\n",old_ptr,__FILE__,__LINE__); \
                        new_ptr = realloc(old_ptr,s); \
          printf("realloc::calloc ptr/size <0x%x: %d bytes> from <%s: line %d> !\n",new_ptr,s,__FILE__,__LINE__); \
                   new_ptr; })



#define free(_ptr) ({                           \
          printf(" free ptr <0x%x:> from <%s: line %d> !\n",_ptr,__FILE__,__LINE__); \
                  free(_ptr);   })


#endif
