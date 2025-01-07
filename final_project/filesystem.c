#include "filesystem.h"

// 共享存儲區域
char storage[1024000]; // 假設storage最大儲存空間大小為 1000KB

void init_filesystem(FileSystem *fs, int size, int storage_start_block) {
    if (size > sizeof(storage) - storage_start_block * BLOCK_SIZE) {
        printf("Error: Partition size exceeds storage capacity.\n");
        return;
    }

    fs->partition_size = size;
    fs->total_blocks = size / BLOCK_SIZE;
    fs->free_blocks = fs->total_blocks;
    fs->storage_start_block = storage_start_block;
    fs->file_count = 0;
    fs->files = NULL;
    fs->used_blocks_bitmask = malloc(fs->total_blocks / 8);
    memset(storage + storage_start_block * BLOCK_SIZE, 0, size); // 清空分區指定區域的記憶體初始化為零(記憶體起始位置, 初始化值, 初始化大小)
    strcpy(fs->current_path, "/"); // 設定根目錄
}

void load_filesystem(FileSystem *fs, const char *filename) {
    // 確保檔案名稱以 ".img" 結尾
    if (!strstr(filename, ".img")) {
        printf("Error: The file '%s' is not a valid .img dump file.\n", filename);
        return;
    }

    FILE *file = fopen(filename, "rb");
    if (file) {
        fread(fs, sizeof(FileSystem), 1, file);
        fs->files = (File *)malloc(fs->file_count * sizeof(File));
        if (fs->files == NULL) {
            printf("Error: Could not allocate memory for files.\n");
            exit(1);
        }
        fread(fs->files, sizeof(File), fs->file_count, file);
        fread(storage + fs->storage_start_block * BLOCK_SIZE, fs->partition_size, 1, file);
        fclose(file);
        printf("Filesystem loaded from '%s'.\n", filename);
    } else {
        printf("Error: Could not load filesystem. Creating new filesystem.\n");
        init_filesystem(fs, BLOCK_SIZE * 100, 0);
    }
}

void save_filesystem(FileSystem *fs, const char *filename) {
    // 確保檔案名稱以 ".img" 結尾
    if (!strstr(filename, ".img")) {
        printf("Error: The file '%s' is not a valid .img dump file.\n", filename);
        return;
    }

    FILE *file = fopen(filename, "wb");
    if (file) {
        fwrite(fs, sizeof(FileSystem), 1, file);
        fwrite(fs->files, sizeof(File), fs->file_count, file);
        fwrite(storage + fs->storage_start_block * BLOCK_SIZE, fs->partition_size, 1, file);
        fclose(file);
        printf("Filesystem saved to '%s'.\n", filename);
    } else {
        printf("Error: Could not save filesystem.\n");
    }
}

//從storage_used_blocks裡找出連續可用的區塊
int find_free_blocks(FileSystem *fs, int required_blocks) {
    int start_block = -1;
    int count = 0;
    for (int i = 0; i < fs->total_blocks; i++) {
        if (!(fs->used_blocks_bitmask[i / 8] & (1 << (i % 8)))) {
            if (start_block == -1) {
                start_block = i;
            }
            count++;
            if (count == required_blocks) {
                return start_block;
            }
        } else {
            start_block = -1;
            count = 0;
        }
    }
    return -1;
}

// 印出bitmask
int print_bitmask(FileSystem *fs) {
    for (int i = 0; i < fs->total_blocks; i++) {
        if (i % 8 == 0) {
            printf(" ");
        }
        printf("%d", (fs->used_blocks_bitmask[i / 8] & (1 << (i % 8))) ? 1 : 0);
    }
    printf("\n");
}

void exit_and_store(FileSystem *fs) {
    char filename[MAX_FILENAME];

    printf("Enter the filename to save the filesystem (e.g., my_filesystem.img): ");
    scanf("%s", filename);

    // 檢查檔案名稱是否以 ".img" 結尾
    if (!strstr(filename, ".img")) {
        printf("Error: The file '%s' is not a valid .img dump file. Adding '.img' extension automatically.\n", filename);
        strcat(filename, ".img"); // 自動補上 ".img"
    }

    save_filesystem(fs, filename);
    printf("Filesystem state saved to '%s'. Exiting.\n", filename);
}

