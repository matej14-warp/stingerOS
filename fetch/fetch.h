#ifndef FETCH_H
#define FETCH_H
int  fetch_package(const char *pkg_name);
void fetch_list_packages(void);
void fetch_update_index(void);
void fetch_set_cdn(const char *host);
#endif
