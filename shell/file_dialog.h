







typedef struct {
    char path[256];
    char filename[128];
    int result;
} file_dialog_result_t;


file_dialog_result_t file_dialog_open(sfs_node_t *start_dir, const char *title);


file_dialog_result_t file_dialog_save(sfs_node_t *start_dir, const char *title, const char *default_name);






