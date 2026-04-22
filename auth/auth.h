












typedef struct {
    char     username[AUTH_NAME_MAX];
    char     home[AUTH_HOME_MAX];
    char     shell[AUTH_SHELL_MAX];
    uint32_t uid;
    uint32_t gid;
    int      locked;
} auth_user_t;


uint32_t auth_hash_password(const char *password);


int auth_verify(const char *username, const char *password, auth_user_t *out);


int auth_add_user(const char *username, const char *password,
                  uint32_t uid, uint32_t gid,
                  const char *home, const char *shell);


void auth_get_hostname(char *buf, size_t len);


int auth_set_hostname(const char *name);


void auth_init_etc(void);


extern auth_user_t current_user;
extern int         user_logged_in;


void login_prompt(void);






