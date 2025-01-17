#include "filesystem.h"
#define ENCRYPTION_KEY 0xAA // 加密使用的簡單密鑰

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



void encrypt(char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        data[i] ^= ENCRYPTION_KEY;
    }
}

void save_filesystem(FileSystem *fs, const char *filename) {
    char password[256];
    printf("Enter password to protect this filesystem: ");
    scanf("%s", password);

    strncpy(fs->password, password, sizeof(fs->password)); // Store password in the filesystem structure

    FILE *file = fopen(filename, "wb");
    if (file) {
        // Encrypt filesystem metadata, including the password
        encrypt((char *)fs, sizeof(FileSystem));
        fwrite(fs, sizeof(FileSystem), 1, file);
        encrypt((char *)fs, sizeof(FileSystem)); // Restore after saving

        // Encrypt file metadata
        encrypt((char *)fs->files, fs->file_count * sizeof(File));
        fwrite(fs->files, sizeof(File), fs->file_count, file);
        encrypt((char *)fs->files, fs->file_count * sizeof(File));

        // Encrypt storage
        encrypt(storage + fs->storage_start_block * BLOCK_SIZE, fs->partition_size);
        fwrite(storage + fs->storage_start_block * BLOCK_SIZE, fs->partition_size, 1, file);
        encrypt(storage + fs->storage_start_block * BLOCK_SIZE, fs->partition_size);

        // Encrypt bitmask
        encrypt((char *)fs->used_blocks_bitmask, fs->total_blocks / 8);
        fwrite(fs->used_blocks_bitmask, fs->total_blocks / 8, 1, file);
        encrypt((char *)fs->used_blocks_bitmask, fs->total_blocks / 8);

        fclose(file);
        printf("Filesystem saved to '%s' with encryption.\n", filename);
    } else {
        printf("Error: Could not save filesystem.\n");
    }
}


void load_filesystem(FileSystem *fs) {
    char filename[MAX_FILENAME];
    char password[256];
    int attempt = 0;

    printf("Enter the filename of the filesystem image: ");
    scanf("%s", filename);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error: The file '%s' does not exist or cannot be opened.\n", filename);
        return;
    }

    fread(fs, sizeof(FileSystem), 1, file);
    encrypt((char *)fs, sizeof(FileSystem)); // Decrypt metadata, including password

    printf("Enter password to decrypt this filesystem (3 attempts max):\n");
    while (attempt < 3) {
        scanf("%s", password);
        if (strcmp(password, fs->password) == 0) { // Compare entered password with stored password
            // Load file metadata
            fs->files = (File *)malloc(fs->file_count * sizeof(File));
            fread(fs->files, sizeof(File), fs->file_count, file);
            encrypt((char *)fs->files, fs->file_count * sizeof(File)); // Decrypt

            // Load storage
            fread(storage + fs->storage_start_block * BLOCK_SIZE, fs->partition_size, 1, file);
            encrypt(storage + fs->storage_start_block * BLOCK_SIZE, fs->partition_size); // Decrypt

            // Load bitmask
            fs->used_blocks_bitmask = malloc(fs->total_blocks / 8);
            fread(fs->used_blocks_bitmask, fs->total_blocks / 8, 1, file);
            encrypt((char *)fs->used_blocks_bitmask, fs->total_blocks / 8); // Decrypt

            fclose(file);
            printf("Filesystem loaded successfully from '%s'.\n", filename);
            return;
        } else {
            attempt++;
            printf("Incorrect password. Attempts left: %d\n", 3 - attempt);
        }
    }

    printf("Error: Password attempts exceeded. Exiting.\n");
    fclose(file);
    exit(EXIT_FAILURE);
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

void set_bitmask(FileSystem *fs, int start_block, int required_blocks) {
    for (int i = 0; i < required_blocks; i++) {
        fs->used_blocks_bitmask[(start_block + i) / 8] |= 1 << ((start_block + i) % 8);
    }
}

void clear_bitmask(FileSystem *fs, int start_block, int required_blocks) {
    for (int i = 0; i < required_blocks; i++) {
        fs->used_blocks_bitmask[(start_block + i) / 8] &= ~(1 << ((start_block + i) % 8));
    }
}

// 印出bitmask
void print_bitmask(FileSystem *fs) {
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

