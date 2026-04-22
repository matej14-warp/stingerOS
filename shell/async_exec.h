







typedef struct {
    thread_t *thread;
    int status;
    int done;
} async_task_t;


async_task_t* async_exec_cmd(const char *cmd);


int async_task_done(async_task_t *task);


void async_task_wait(async_task_t *task);


void async_task_free(async_task_t *task);


int async_task_status(async_task_t *task);






