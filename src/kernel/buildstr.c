// Return a string unique to this build.
const char* buildstr() {
    return __TIME__ " " __DATE__;
}
