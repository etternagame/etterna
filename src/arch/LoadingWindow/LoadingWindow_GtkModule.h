#ifndef LOADING_WINDOW_MODULE_GTK
#define LOADING_WINDOW_MODULE_GTK

struct RageSurface;

typedef const char* (*INIT)(int* argc, char*** argv);
typedef void (*SHUTDOWN)();
typedef void (*SETTEXT)(const char* s);
typedef void (*SETICON)(const RageSurface* pSrcImg);
typedef void (*SETSPLASH)(const RageSurface* pSplash);
typedef void (*SETPROGRESS)(int progress, int totalWork);
typedef void (*SETINDETERMINATE)(bool indeterminate);

#endif
