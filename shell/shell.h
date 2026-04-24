/* scorpion OS - shell/shell.h */
#ifndef SHELL_H
#define SHELL_H
#include "../fs/sfs.h"
#include "../auth/auth.h"   /* auth_user_t, current_user */

void shell_init(void);
void shell_run(void);
void shell_exec_cmd(const char *cmd);
void shell_exec_single(const char *cmd);
void shell_set_prompt(const char *fmt);

/* Pipe support */
void pipe_execute(const char *cmd1, const char *cmd2);
void sfs_write_pipe_tmp(const uint8_t *data, size_t len);

/* Environment */
void        env_set(const char *key, const char *val);
const char *env_get(const char *key);
void        env_unset(const char *key);
void        cmd_env_cmd(void);

/* Login — defined in auth.c */
void login_prompt(void);

#endif /* SHELL_H */
