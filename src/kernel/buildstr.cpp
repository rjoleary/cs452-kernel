#define XSTR(a) STR(a)
#define STR(a) #a

// Return a string unique to this build.
const char* buildstr() {
    return __TIME__
        " " __DATE__
#ifdef CACHE_ENABLED
        " CACHE=" XSTR(CACHE_ENABLED)
#endif
#ifdef OPT_ENABLED
        " OPT=" XSTR(OPT_ENABLED)
#endif
#ifdef PROF_INTERVAL
        " PROF=" XSTR(PROF_INTERVAL)
#endif
        ;
}
