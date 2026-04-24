/* hello-world module for scorpion OS
   This C file is part of the hello-world package.
   It would be compiled by a scorpion runtime compiler.  */

/* Example scorpion module API (future) */

const char *module_name    = "hello-world";
const char *module_version = "1.0";
const char *module_author  = "scorpion";

void module_main(void) {
    /* In a future version of scorpion, this would be
       compiled and loaded as a dynamic module.
       For now it's installed as a source file.        */
    return;
}
