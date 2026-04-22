








typedef struct {
    char *cmd;
    async_task_t *task;
} async_exec_ctx_t;

static void async_exec_worker(void *arg) {
    async_exec_ctx_t *ctx = (async_exec_ctx_t *)arg;
    if (!ctx) return;


    extern void shell_exec_single(const char *cmd);
    shell_exec_single(ctx->cmd);


    ctx->task->done = 1;
    ctx->task->status = 0;
}

async_task_t* async_exec_cmd(const char *cmd) {
    if (!cmd || !cmd[0]) return NULL;

    async_task_t *task = (async_task_t *)kmalloc(sizeof(async_task_t));
    if (!task) return NULL;

    async_exec_ctx_t *ctx = (async_exec_ctx_t *)kmalloc(sizeof(async_exec_ctx_t));
    if (!ctx) {
        kfree(task);
        return NULL;
    }


    int cmd_len = 0;
    while (cmd[cmd_len]) cmd_len++;

    ctx->cmd = (char *)kmalloc(cmd_len + 1);
    if (!ctx->cmd) {
        kfree(ctx);
        kfree(task);
        return NULL;
    }

    int i = 0;
    while (i <= cmd_len) {
        ctx->cmd[i] = cmd[i];
        i++;
    }

    ctx->task = task;
    task->done = 0;
    task->status = -1;


    task->thread = thread_create("async_cmd", async_exec_worker, ctx);
    if (!task->thread) {
        kfree(ctx->cmd);
        kfree(ctx);
        kfree(task);
        return NULL;
    }

    return task;
}

int async_task_done(async_task_t *task) {
    if (!task) return 1;
    return task->done;
}

void async_task_wait(async_task_t *task) {
    if (!task || !task->thread) return;


    volatile int *done = (volatile int *)&task->done;
    while (!*done) {
        __asm__ volatile("hlt");
    }
}

void async_task_free(async_task_t *task) {
    if (!task) return;
    async_task_wait(task);
    kfree(task->thread);
    kfree(task);
}

int async_task_status(async_task_t *task) {
    if (!task) return -1;
    return task->status;
}




