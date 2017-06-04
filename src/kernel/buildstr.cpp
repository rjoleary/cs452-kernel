#define XSTR(a) STR(a)
#define STR(a) #a

// Return a string unique to this build.
const char* buildstr() {
    return __TIME__
        " " __DATE__
#ifdef STRACE_ENABLED
        " STRACE=" XSTR(STRACE_ENABLED)
#endif
#ifdef CACHE_ENABLED
        " CACHE=" XSTR(CACHE_ENABLED)
#endif
#ifdef OPT_ENABLED
        " OPT=" XSTR(OPT_ENABLED)
#endif
        ;
}
