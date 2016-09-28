//#define HAS_SERIAL
#ifdef HAS_SERIAL
#define DEBUG_TASK(x) putstr(x)
#define DEBUG_RFIO(x) putstr(x)
#define DEBUG32_RFIO(x) put_hex32(x)
#define DEBUG_INFO(x) putstr(x)
#define DEBUG32_INFO(x) put_hex32(x)
#define DEBUG_UI(x) putstr(x)
#define DEBUG32_UI(x) put_hex32(x)
#else
#define DEBUG_TASK(x) 
#define DEBUG_RFIO(x)
#define DEBUG32_RFIO(x)
#define DEBUG_INFO(x)
#define DEBUG32_INFO(x) 
#define DEBUG_UI(x) 
#define DEBUG32_UI(x) 
#endif
