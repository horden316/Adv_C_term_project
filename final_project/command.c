#include "command.h"

void ls(FileSystem *fs) {
    printf("\033[1;34m[Directory]\033[0m   \033[0;32m[File]\033[0m\n");
    for (int i = 0; i < fs->file_count; i++) {
        // 只顯示當前目錄下的檔案和目錄
        if (fs->files[i].used_blocks > 0 && 
            strcmp(fs->files[i].parent_name, fs->current_path) == 0) {
            if (fs->files[i].is_directory) {
                printf("\033[1;34m%s\033[0m\n", fs->files[i].name); // 藍色是目錄
            }
            else if (fs->files[i].is_directory == 0)
            {
                printf("\033[0;32m%s (%d bytes)\033[0m\n", fs->files[i].name, fs->files[i].size);// 綠色是檔案
            }

        }
    }
    print_bitmask(fs);
}


void mkdir(FileSystem *fs, const char *dirname) {
    // 檢查目錄是否已存在
    for (int i = 0; i < fs->file_count; i++) {
        if (fs->files[i].used_blocks > 0 && strcmp(fs->files[i].name, dirname) == 0 && strcmp(fs->files[i].parent_name, fs->current_path) == 0) {
            printf("Error: Directory '%s' already exists.\n", dirname);
            return;
        }
    }

    // 檢查是否有足夠的空間來創建新目錄
    if (fs->free_blocks < 1) {
        printf("Error: Not enough space to create directory '%s'.\n", dirname);
        return;
    }

    // 獲取位置
    int start_block = -1;
    start_block = find_free_blocks(fs, 1);

    // 動態分配新的文件結構
    fs->files = (File *)realloc(fs->files, (fs->file_count + 1) * sizeof(File));
    if (fs->files == NULL) {
        printf("Error: Could not allocate memory for new directory.\n");
        return;
    }

    // 初始化新目錄
    File *new_dir = &fs->files[fs->file_count];
    strncpy(new_dir->name, dirname, MAX_FILENAME);
    new_dir->size = 0;
    new_dir->start_block = start_block;
    new_dir->used_blocks = 1; // 目錄至少佔用一個區塊
    new_dir->is_directory = 1; // 標記為目錄
    strncpy(new_dir->parent_name, fs->current_path, MAX_FILENAME); // 設定父目錄名稱

    // 更新bitmask
    set_bitmask(fs, start_block, 1);


    fs->file_count++;
    fs->free_blocks--;

    printf("Directory '%s' created.\n", dirname);
    //print_bitmask(fs);
}

void rmdir(FileSystem *fs, const char *dirname) {
    // 檢查目錄是否存在
    for (int i = 0; i < fs->file_count; i++) {
        if (fs->files[i].used_blocks > 0 && 
            fs->files[i].is_directory && 
            strcmp(fs->files[i].name, dirname) == 0 && // 檢查是否為目標目錄
            strcmp(fs->files[i].parent_name, fs->current_path) == 0) { // 檢查是否在當前目錄下
            
            // 組合完整路徑
            char full_path[MAX_PATH];
            if (strcmp(fs->current_path, "/") == 0) {
                snprintf(full_path, MAX_PATH, "/%s", dirname);
            } else {
                snprintf(full_path, MAX_PATH, "%s/%s", fs->current_path, dirname);
            }

            // 檢查此目錄有沒有children
            for (int j = 0; j < fs->file_count; j++) {
                if (fs->files[j].used_blocks > 0) {
                    if (strcmp(full_path, fs->files[j].parent_name) == 0) {
                        printf("Error: Directory '%s' is not empty.\n", dirname);
                        return;
                    }
                }
            }

            // 移除目錄
            fs->files[i].used_blocks = 0;
            fs->free_blocks++;

            // 更新bitmask
            clear_bitmask(fs, fs->files[i].start_block, 1);

            // 移除File struct並將後面的元素往前遞補
            for (int j = i; j < fs->file_count - 1; j++) {
                fs->files[j] = fs->files[j + 1];
            }
            fs->file_count--;
            fs->files = (File *)realloc(fs->files, fs->file_count * sizeof(File));

            printf("Directory '%s' removed.\n", dirname);
            return;
        }
    }

    printf("Error: Directory '%s' not found in current directory.\n", dirname);
}


void cd(FileSystem *fs, const char *path) {
    if (strcmp(path, "..") == 0) { //
        // 返回上一層目錄
        char *last_slash = strrchr(fs->current_path, '/'); // 找到最後一個斜線
        if (last_slash && last_slash != fs->current_path) { // 如果不是根目錄
            *last_slash = '\0'; // 移除最後一個斜線 '\0'代表結束
        } else {
            strcpy(fs->current_path, "/");
        }
    } else {
        // 檢查目錄是否存在
        for (int i = 0; i < fs->file_count; i++) {
            if (fs->files[i].used_blocks > 0 && // 第i個檔案是否存在
                fs->files[i].is_directory && // 是否為目錄
                strcmp(fs->files[i].name, path) == 0) { // 是否為指定目錄

                char temp_path[MAX_FILENAME];
                size_t path_len;
                
                // 根目錄特殊處理
                if (strcmp(fs->current_path, "/") == 0) {
                    path_len = snprintf(temp_path, MAX_FILENAME, "/%s", path); //snprintf() 函數用來格式化輸出字串， 用法：snprintf(char *str, size_t size, const char *format, ...);
                } else {
                    path_len = snprintf(temp_path, MAX_FILENAME, "%s/%s", fs->current_path, path);
                }

                if (path_len >= MAX_FILENAME) { // 檢查超過路徑長度限制
                    printf("error：Exceed path lenth limitation\n");
                    return;
                }

                strcpy(fs->current_path, temp_path);
                printf("Current directory: %s\n", fs->current_path);
                return;
            }
        }
        printf("Error: Directory '%s' not found.\n", path);
    }
}

void put(FileSystem *fs, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file '%s'.\n", filename);
        return;
    }

    fseek(file, 0, SEEK_END);
    int filesize = ftell(file); // ftell() 函數用來得到文件指標的當前位置
    fseek(file, 0, SEEK_SET);

    //處理同檔名問題
    for (int i = 0; i < fs->file_count; i++) {
        if (strcmp(fs->files[i].name, filename) == 0 &&
            strcmp(fs->files[i].parent_name, fs->current_path) == 0) {
            printf("Error: File '%s' already exists in the current directory.\n", filename);
            fclose(file);
            return;
        }
    }


    int required_blocks = (filesize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (required_blocks > fs->free_blocks) {
        printf("Error: Not enough space to store file '%s'.\n", filename);
        fclose(file);
        return;
    }

    fs->files = (File *)realloc(fs->files, (fs->file_count + 1) * sizeof(File));
    if (fs->files == NULL) {
        printf("Error: Could not allocate memory for new file.\n");
        fclose(file);
        return;
    }

    // 檢查bitmask以找到足夠塞得下file的連續空間
    int start_block = -1;
    start_block = find_free_blocks(fs, required_blocks);


    if (start_block == -1) {
        printf("Error: Not enough continuous space to store file '%s'.\n", filename);
        fclose(file);
        return;
    }

    // 更新bitmask
    set_bitmask(fs, start_block, required_blocks);


    File *new_file = &fs->files[fs->file_count];
    strncpy(new_file->name, filename, MAX_FILENAME);
    new_file->size = filesize;
    new_file->used_blocks = required_blocks;
    new_file->start_block = start_block;
    new_file->is_directory = 0;
    strcpy(new_file->parent_name, fs->current_path);

    fread(storage + start_block * BLOCK_SIZE, filesize, 1, file);
    fs->free_blocks -= required_blocks;
    fs->file_count++;
    fclose(file);
    printf("File '%s' added to filesystem.\n", filename);
}

void get(FileSystem *fs, const char *filename) {
    // 檢查 dump 資料夾是否存在，若不存在則建立
    FILE *dir = fopen("dump", "r");
    if (!dir) { // 如果無法開啟，假設資料夾不存在
        system("mkdir dump"); // 使用系統指令建立資料夾
    } else {
        fclose(dir); // 如果能開啟，說明資料夾存在，關閉檔案
    }

    for (int i = 0; i < fs->file_count; i++) {
        if (strcmp(fs->files[i].name, filename) == 0 &&
            strcmp(fs->files[i].parent_name, fs->current_path) == 0) { // 檢查檔案是否在當前目錄
            // 構建完整的輸出路徑：dump/filename
            char output_path[MAX_FILENAME + 5];
            snprintf(output_path, sizeof(output_path), "dump/%s", filename);

            // 打開 OS 檔案系統中的檔案進行寫入
            FILE *file = fopen(output_path, "w");
            if (!file) {
                printf("Error: Could not create file '%s'.\n", output_path);
                return;
            }

            // 從虛擬檔案系統讀取內容並寫入到檔案
            fwrite(storage + (fs->files[i].start_block * BLOCK_SIZE), fs->files[i].size, 1, file);

            fclose(file);
            printf("File '%s' retrieved from filesystem to '%s'.\n", filename, output_path);
            return;
        }
    }

    printf("Error: File '%s' not found in the current directory.\n", filename);
}



void rm(FileSystem *fs, const char *filename) {
    for (int i = 0; i < fs->file_count; i++) {
        if (strcmp(fs->files[i].name, filename) == 0 && strcmp(fs->files[i].parent_name, fs->current_path) == 0) {
            fs->free_blocks += fs->files[i].used_blocks;
            // 更新bitmask
            clear_bitmask(fs, fs->files[i].start_block, fs->files[i].used_blocks);

            // 移除File struct並將後面的元素往前遞補
            for (int j = i; j < fs->file_count - 1; j++) {
                fs->files[j] = fs->files[j + 1];
            }
            fs->file_count--;
            fs->files = (File *)realloc(fs->files, fs->file_count * sizeof(File));
            printf("File '%s' removed from filesystem.\n", filename);
            return;
        }
    }

    printf("Error: File '%s' not found.\n", filename);
}

void cat(FileSystem *fs, const char *filename) {
    for (int i = 0; i < fs->file_count; i++) {
        if (strcmp(fs->files[i].name, filename) == 0 &&
            strcmp(fs->files[i].parent_name, fs->current_path) == 0) {
            printf("File '%s' content:\n", filename);
            for (int j = 0; j < fs->files[i].size; j++) {
                 printf("%c", storage[fs->files[i].start_block * BLOCK_SIZE + j]);
            }

            printf("\n");
            return;
        }
    }

    // 檔案不存在
    printf("Error: File '%s' not found in the current directory.\n", filename);

}

void status(FileSystem *fs) {
    int used_blocks = 0, file_blocks = 0;
     for (int i = 0; i < fs->file_count; i++) {
         if (fs->files[i].used_blocks > 0) {
             used_blocks += fs->files[i].used_blocks;
             file_blocks += (fs->files[i].size + BLOCK_SIZE - 1) / BLOCK_SIZE;
         }
     }

    printf("partition size: %d\n", fs->partition_size);
    printf("total blocks: %d\n", fs->total_blocks);
    printf("used blocks: %d\n", used_blocks);
    printf("files' blocks: %d\n", file_blocks);
    printf("block size: %d\n", BLOCK_SIZE);
    printf("free space: %d\n", fs->partition_size - (used_blocks * BLOCK_SIZE));
}


void help() {
    printf("List of commands:\n");
    printf("'ls'      list directory\n");
    printf("'cd'      change directory\n");
    printf("'rm'      remove file\n");
    printf("'mkdir'   make directory\n");
    printf("'rmdir'   remove directory\n");
    printf("'put'     put file into the space\n");
    printf("'get'     get file from the space\n");
    printf("'cat'     show content of a file\n");
    printf("'status'  show status of the space\n");
    printf("'create'  create a new text file\n");
    printf("'edit'    edit an existing text file\n");
    printf("'help'    list commands\n");
    printf("'exit'    exit and save filesystem\n");
}
void create(FileSystem *fs, const char *filename) {
    // 檢查是否已存在同名文件
    for (int i = 0; i < fs->file_count; i++) {
        if (strcmp(fs->files[i].name, filename) == 0 &&
            strcmp(fs->files[i].parent_name, fs->current_path) == 0) {
            printf("Error: File '%s' already exists in the current directory.\n", filename);
            return;
        }
    }

    printf("Enter text content for the file '%s' (end with an empty line):\n", filename);
    char content[1024];
    char line[256];
    content[0] = '\0';

    // 清除輸入緩衝區，避免殘留字符影響輸入
    while (getchar() != '\n'); 

    while (fgets(line, sizeof(line), stdin)) {
        // 檢查是否為空行來結束輸入
        if (strcmp(line, "\n") == 0) break;
        strcat(content, line);
    }

    int filesize = strlen(content);
    if (filesize == 0) {
        printf("Error: File '%s' is empty. Please provide content.\n", filename);
        return;
    }

    int required_blocks = (filesize + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // 檢查是否有足夠的空間來存儲文件
    if (required_blocks > fs->free_blocks) {
        printf("Error: Not enough space to store file '%s'.\n", filename);
        return;
    }

    // 尋找空閒的區塊來存儲文件
    int start_block = find_free_blocks(fs, required_blocks);
    if (start_block == -1) {
        printf("Error: Not enough continuous space to store file '%s'.\n", filename);
        return;
    }

    // 動態分配新的文件結構
    fs->files = (File *)realloc(fs->files, (fs->file_count + 1) * sizeof(File));
    if (fs->files == NULL) {
        printf("Error: Could not allocate memory for new file.\n");
        return;
    }

    // 初始化新文件
    File *new_file = &fs->files[fs->file_count];
    strncpy(new_file->name, filename, MAX_FILENAME);
    new_file->size = filesize;
    new_file->start_block = start_block;
    new_file->used_blocks = required_blocks;
    new_file->is_directory = 0;
    strncpy(new_file->parent_name, fs->current_path, MAX_FILENAME);

    // 更新位元遮罩並寫入文件內容到存儲空間
    set_bitmask(fs, start_block, required_blocks);
    memcpy(storage + start_block * BLOCK_SIZE, content, filesize);

    fs->free_blocks -= required_blocks;
    fs->file_count++;

    printf("Text file '%s' created successfully.\n", filename);
}



void edit(FileSystem *fs, const char *filename) {
    // 找到文件在文件系統中的位置
    int file_index = -1;
    for (int i = 0; i < fs->file_count; i++) {
        if (strcmp(fs->files[i].name, filename) == 0 &&
            strcmp(fs->files[i].parent_name, fs->current_path) == 0) {
            file_index = i;
            break;
        }
    }
    if (file_index == -1) {
        printf("Error: File '%s' not found in the current directory.\n", filename);
        return;
    }

    File *file = &fs->files[file_index];

    // 清空緩衝區避免干擾
    while (getchar() != '\n');

    // 編輯文件內容
    printf("Enter new content for the file '%s' (end with an empty line):\n", filename);
    char content[1024] = {0};
    char line[256];
    int new_filesize = 0;

    // 開始讀取用戶輸入
    while (fgets(line, sizeof(line), stdin)) {
        // 空行結束輸入
        if (strcmp(line, "\n") == 0) break;

        // 檢查內容大小是否超出限制
        if (new_filesize + strlen(line) >= sizeof(content)) {
            printf("Error: Content exceeds maximum allowed size.\n");
            return;
        }

        strcat(content, line);
        new_filesize += strlen(line);
    }

    // 檢查是否輸入了內容
    if (new_filesize == 0) {
        printf("Error: New content for file '%s' is empty. File content remains unchanged.\n", filename);
        return;
    }

    int new_required_blocks = (new_filesize + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // 檢查是否需要更多空間
    if (new_required_blocks > file->used_blocks) {
        int additional_blocks = new_required_blocks - file->used_blocks;

        // 檢查是否有足夠的空間
        if (additional_blocks > fs->free_blocks) {
            printf("Error: Not enough space to update file '%s'.\n", filename);
            return;
        }

        // 嘗試找到額外的連續區塊
        int start_block = find_free_blocks(fs, additional_blocks);
        if (start_block == -1) {
            printf("Error: Not enough continuous space to update file '%s'.\n", filename);
            return;
        }

        // 更新位元遮罩
        set_bitmask(fs, start_block, additional_blocks);

        // 將額外區塊追加到現有區塊後面
        memcpy(storage + (start_block * BLOCK_SIZE),
               content + (file->used_blocks * BLOCK_SIZE),
               additional_blocks * BLOCK_SIZE);

        fs->free_blocks -= additional_blocks;
        file->used_blocks = new_required_blocks;
    }

    // 更新文件內容
    memcpy(storage + (file->start_block * BLOCK_SIZE), content, new_filesize);
    file->size = new_filesize;

    printf("Content of file '%s' updated successfully.\n", filename);

    // 提示是否更改文件名
    printf("Do you want to rename the file?\n");
    printf("1. Keep the current name: '%s'\n", filename);
    printf("2. Rename the file\n");
    printf("Enter your choice (1 or 2): ");

    int choice;
    scanf("%d", &choice);
    getchar(); // 清除緩衝區中的換行符

    if (choice == 2) {
        printf("Enter a new filename (with .txt extension): ");
        char new_filename[MAX_FILENAME];
        fgets(new_filename, sizeof(new_filename), stdin);

        // 去掉輸入中的換行符
        new_filename[strcspn(new_filename, "\n")] = '\0';

        // 檢查新名稱是否與其他文件衝突
        for (int i = 0; i < fs->file_count; i++) {
            if (strcmp(fs->files[i].name, new_filename) == 0 &&
                strcmp(fs->files[i].parent_name, fs->current_path) == 0) {
                printf("Error: File '%s' already exists in the current directory.\n", new_filename);
                return;
            }
        }

        // 更新文件名
        strncpy(file->name, new_filename, MAX_FILENAME);
        printf("File renamed to '%s'.\n", new_filename);
    } else {
        printf("Filename remains unchanged: '%s'.\n", filename);
    }

    printf("Text file '%s' edited successfully.\n", file->name);
}



