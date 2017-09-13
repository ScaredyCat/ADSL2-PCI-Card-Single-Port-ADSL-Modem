#ifdef	__cplusplus
extern "C" {
#endif

void
stunGetExtAddr(const char* serveraddrstr, char* tmpaddr);

unsigned short
stunGetExtPort(const char* serveraddrstr, unsigned short localport);


#ifdef __cplusplus
}
#endif

