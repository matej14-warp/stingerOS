





void shell_init(void);
void shell_run(void);
void shell_exec_cmd(const char *cmd);
void shell_exec_single(const char *cmd);
void shell_set_prompt(const char *fmt);


void pipe_execute(const char *cmd1, const char *cmd2);
void sfs_write_pipe_tmp(const uint8_t *data, size_t len);


void        env_set(const char *key, const char *val);
const char *env_get(const char *key);
void        env_unset(const char *key);
void        cmd_env_cmd(void);


void login_prompt(void);






