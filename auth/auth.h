#ifndef AUTH_H
#define AUTH_H

/* scorpion OS - auth/auth.h
   User authentication against /etc/passwd
   /etc/passwd format (one user per line):
     username:password_hash:uid:gid:home:shell
   Hash: djb2 XOR fold, printed as 8 hex digits.
   Special: password "!" means account locked.             */

#include <stdint.h>
#include "../fs/sfs.h"

#define AUTH_MAX_USERS   16
#define AUTH_NAME_MAX    32
#define AUTH_HOME_MAX    64
#define AUTH_SHELL_MAX   32

typedef struct {
    char     username[AUTH_NAME_MAX];
    char     home[AUTH_HOME_MAX];
    char     shell[AUTH_SHELL_MAX];
    uint32_t uid;
    uint32_t gid;
    int      locked;        /* 1 if password is "!" */
} auth_user_t;

/* Hash a plaintext password → 32-bit djb2 XOR fold */
uint32_t auth_hash_password(const char *password);

/* Verify username+password against /etc/passwd. Returns 1 on success. */
int auth_verify(const char *username, const char *password, auth_user_t *out);

/* Add or overwrite a user entry in /etc/passwd */
int auth_add_user(const char *username, const char *password,
                  uint32_t uid, uint32_t gid,
                  const char *home, const char *shell);

/* Read current hostname from /etc/hostname */
void auth_get_hostname(char *buf, size_t len);

/* Write hostname to /etc/hostname */
int auth_set_hostname(const char *name);

/* Populate /etc/ with sensible defaults if files are missing */
void auth_init_etc(void);

/* Session: currently logged-in user (set by login_prompt) */
extern auth_user_t current_user;
extern int         user_logged_in;

/* Boot-time login prompt — blocks until valid login */
void login_prompt(void);

#endif /* AUTH_H */
