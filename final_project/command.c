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
    // Check if the file exists and is not a directory
    for (int i = 0; i < fs->file_count; i++) {
        if (strcmp(fs->files[i].name, filename) == 0 &&
            strcmp(fs->files[i].parent_name, fs->current_path) == 0 &&
            !fs->files[i].is_directory) {

            // Load existing content into editable_content
            char editable_content[1024];
            memset(editable_content, 0, sizeof(editable_content));
            memcpy(editable_content, storage + fs->files[i].start_block * BLOCK_SIZE, fs->files[i].size);
            editable_content[fs->files[i].size] = '\0'; // Null-terminate for safety

            printf("Editing '%s'.\n", filename);
            printf("--- Current Content Below ---\n");
            printf("%s\n", editable_content); // Display the existing content

            char new_content[1024];
            memset(new_content, 0, sizeof(new_content)); // Initialize new content buffer

            printf("\n--- Modify the content below. Enter the updated content line by line ---\n");
            printf("--- Current content is preloaded. Leave blank to keep, type '-d' to delete. ---\n");

            char line[256]; // Buffer for user input
            char *line_ptr = strtok(editable_content, "\n"); // Tokenize original content by lines
            while (getchar() != '\n'); 
            // Loop through each line of the original content
            while (line_ptr) {
                printf("Original: %s\nEdit (leave blank to keep, type '-d' to delete): ", line_ptr);
                fgets(line, sizeof(line), stdin);

                // Remove trailing newline from input
                line[strcspn(line, "\n")] = '\0';

                if (strcmp(line, "-d") == 0) {
                    // If user types "-d", skip this line (delete it)
                    printf("Line deleted.\n");
                } else if (strcmp(line, "") == 0) {
                    // If user leaves input blank, keep the original line
                    strcat(new_content, line_ptr);
                    strcat(new_content, "\n");
                } else {
                    // Replace with new content
                    strcat(new_content, line);
                    strcat(new_content, "\n");
                }

                line_ptr = strtok(NULL, "\n"); // Move to the next line
            }

            // Allow user to add additional lines
            printf("Add additional lines (end with an empty line):\n");
            while (fgets(line, sizeof(line), stdin)) {
                if (strcmp(line, "\n") == 0) break; // Stop on empty line
                strcat(new_content, line); // Append additional content
            }

            int new_size = strlen(new_content);
            if (new_size == 0) {
                printf("Error: New content is empty. Editing aborted.\n");
                return;
            }

            // Ask the user whether to save as original or new file
            char save_choice[10];
            printf("Do you want to save changes as the original file '%s'? (yes/no): ", filename);
            fgets(save_choice, sizeof(save_choice), stdin);
            save_choice[strcspn(save_choice, "\n")] = '\0'; // Remove newline

            if (strcmp(save_choice, "no") == 0) {
                // Ask for a new filename
                char new_filename[MAX_FILENAME];
                printf("Enter new filename: ");
                fgets(new_filename, sizeof(new_filename), stdin);
                new_filename[strcspn(new_filename, "\n")] = '\0'; // Remove newline

                // Check if the new filename already exists in the current directory
                for (int j = 0; j < fs->file_count; j++) {
                    if (strcmp(fs->files[j].name, new_filename) == 0 &&
                        strcmp(fs->files[j].parent_name, fs->current_path) == 0) {
                        printf("Error: File '%s' already exists in the current directory.\n", new_filename);
                        return;
                    }
                }

                // Create a new file with the new content
                int required_blocks = (new_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
                if (required_blocks > fs->free_blocks) {
                    printf("Error: Not enough space to create new file '%s'.\n", new_filename);
                    return;
                }

                int start_block = find_free_blocks(fs, required_blocks);
                if (start_block == -1) {
                    printf("Error: Not enough continuous space to create new file '%s'.\n", new_filename);
                    return;
                }

                fs->files = (File *)realloc(fs->files, (fs->file_count + 1) * sizeof(File));
                if (!fs->files) {
                    printf("Error: Memory allocation failed.\n");
                    return;
                }

                File *new_file = &fs->files[fs->file_count];
                strncpy(new_file->name, new_filename, MAX_FILENAME);
                new_file->size = new_size;
                new_file->start_block = start_block;
                new_file->used_blocks = required_blocks;
                new_file->is_directory = 0;
                strncpy(new_file->parent_name, fs->current_path, MAX_FILENAME);

                set_bitmask(fs, start_block, required_blocks);
                memcpy(storage + start_block * BLOCK_SIZE, new_content, new_size);

                fs->free_blocks -= required_blocks;
                fs->file_count++;

                printf("File '%s' created successfully.\n", new_filename);
                return;
            }

            // Overwrite the original file
            int required_blocks = (new_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
            if (required_blocks > fs->files[i].used_blocks) {
                // Check if there is enough free space
                if (required_blocks - fs->files[i].used_blocks > fs->free_blocks) {
                    printf("Error: Not enough space to update file '%s'.\n", filename);
                    return;
                }

                // Find new continuous space for the file
                int start_block = find_free_blocks(fs, required_blocks);
                if (start_block == -1) {
                    printf("Error: Not enough continuous space to update file '%s'.\n", filename);
                    return;
                }

                // Free the old space and update bitmask
                clear_bitmask(fs, fs->files[i].start_block, fs->files[i].used_blocks);
                fs->free_blocks += fs->files[i].used_blocks;

                // Allocate new space
                set_bitmask(fs, start_block, required_blocks);
                fs->files[i].start_block = start_block;
                fs->files[i].used_blocks = required_blocks;
                fs->free_blocks -= required_blocks;
            }

            // Write the new content back to the original file
            memcpy(storage + fs->files[i].start_block * BLOCK_SIZE, new_content, new_size);
            fs->files[i].size = new_size;

            printf("File '%s' updated successfully.\n", filename);
            return;
        }
    }

    printf("Error: File '%s' not found in the current directory.\n", filename);
}




















