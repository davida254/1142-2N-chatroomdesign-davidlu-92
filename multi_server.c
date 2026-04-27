#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include <ctype.h>

#define MAX_CLIENTS 20
#define BUFFER_SIZE 1024
#define NAME_SIZE 32
#define ADMIN_PASSWORD "1234"
#define HISTORY_FILE "chat_history.txt"

// mute_until: 0 means not muted, -1 means muted forever, otherwise unix timestamp.
typedef struct {
    int fd;
    char name[NAME_SIZE];
    int is_admin;
    long mute_until;
} Client;

Client clients[MAX_CLIENTS];

void trim_newline(char *s) {
    s[strcspn(s, "\r\n")] = '\0';
}

void now_string(char *out, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(out, size, "%Y-%m-%d %H:%M:%S", t);
}

void save_history(const char *message) {
    FILE *fp = fopen(HISTORY_FILE, "a");
    if (!fp) {
        perror("open history failed");
        return;
    }
    char ts[32];
    now_string(ts, sizeof(ts));
    fprintf(fp, "[%s] %s\n", ts, message);
    fclose(fp);
}

void send_to_fd(int fd, const char *message) {
    send(fd, message, strlen(message), 0);
}

void broadcast_message(const char *message) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0) {
            send_to_fd(clients[i].fd, message);
        }
    }
}

int find_client_index_by_fd(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == fd) return i;
    }
    return -1;
}

int find_client_index_by_name(const char *name) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0 && strcmp(clients[i].name, name) == 0) return i;
    }
    return -1;
}

void remove_client(int idx) {
    if (idx < 0 || idx >= MAX_CLIENTS || clients[idx].fd <= 0) return;

    char msg[BUFFER_SIZE];
    snprintf(msg, sizeof(msg), "[系統] %s 已離開聊天室\n", clients[idx].name);
    printf("%s", msg);
    save_history(msg);

    close(clients[idx].fd);
    clients[idx].fd = 0;
    clients[idx].name[0] = '\0';
    clients[idx].is_admin = 0;
    clients[idx].mute_until = 0;

    broadcast_message(msg);
}

void send_history(int fd, int last_n) {
    FILE *fp = fopen(HISTORY_FILE, "r");
    if (!fp) {
        send_to_fd(fd, "[系統] 目前沒有歷史訊息。\n");
        return;
    }

    char lines[100][BUFFER_SIZE];
    int count = 0;
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), fp)) {
        strncpy(lines[count % 100], line, BUFFER_SIZE - 1);
        lines[count % 100][BUFFER_SIZE - 1] = '\0';
        count++;
    }
    fclose(fp);

    int start = count > last_n ? count - last_n : 0;
    send_to_fd(fd, "========== 最近歷史訊息 ==========" "\n");
    for (int i = start; i < count; i++) {
        send_to_fd(fd, lines[i % 100]);
    }
    send_to_fd(fd, "==================================\n");
}

void create_snapshot(int fd) {
    char ts[32];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", t);

    char filename[64];
    snprintf(filename, sizeof(filename), "snapshot_%s.txt", ts);

    FILE *in = fopen(HISTORY_FILE, "r");
    FILE *out = fopen(filename, "w");
    if (!out) {
        send_to_fd(fd, "[系統] 建立截圖快照失敗。\n");
        if (in) fclose(in);
        return;
    }

    fprintf(out, "聊天室文字截圖 / 快照\n產生時間：%s\n\n", ts);
    if (in) {
        char line[BUFFER_SIZE];
        while (fgets(line, sizeof(line), in)) {
            fputs(line, out);
        }
        fclose(in);
    } else {
        fprintf(out, "目前尚無聊天紀錄。\n");
    }
    fclose(out);

    char msg[BUFFER_SIZE];
    snprintf(msg, sizeof(msg), "[系統] 已建立聊天截圖快照：%s\n", filename);
    send_to_fd(fd, msg);
}

void list_users(int fd) {
    send_to_fd(fd, "========== 線上使用者 ==========" "\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0) {
            char line[BUFFER_SIZE];
            char status[128] = "";
            if (clients[i].is_admin) strcat(status, " 管理員");
            if (clients[i].mute_until == -1) strcat(status, " 永久禁言");
            else if (clients[i].mute_until > time(NULL)) strcat(status, " 暫時禁言");
            snprintf(line, sizeof(line), "- %s%s\n", clients[i].name, status);
            send_to_fd(fd, line);
        }
    }
    send_to_fd(fd, "===============================\n");
}

void show_help(int fd) {
    const char *help =
        "可用指令：\n"
        "/help                  查看指令\n"
        "/history               查看最近歷史訊息\n"
        "/snapshot              建立聊天文字截圖快照\n"
        "/users                 查看線上使用者\n"
        "/admin 1234            取得管理員權限\n"
        "/mute 名稱 秒數        禁言指定秒數，例如 /mute David 60\n"
        "/mute 名稱 forever     永久禁言，但仍可留在聊天室觀看\n"
        "/unmute 名稱           解除禁言\n"
        "/kick 名稱             踢出使用者\n"
        "exit                   離開聊天室\n";
    send_to_fd(fd, help);
}

void handle_command(int idx, char *buffer) {
    char cmd[64], arg1[NAME_SIZE], arg2[64];
    cmd[0] = arg1[0] = arg2[0] = '\0';
    sscanf(buffer, "%63s %31s %63s", cmd, arg1, arg2);

    if (strcmp(cmd, "/help") == 0) {
        show_help(clients[idx].fd);
    } else if (strcmp(cmd, "/history") == 0) {
        send_history(clients[idx].fd, 20);
    } else if (strcmp(cmd, "/snapshot") == 0) {
        create_snapshot(clients[idx].fd);
    } else if (strcmp(cmd, "/users") == 0) {
        list_users(clients[idx].fd);
    } else if (strcmp(cmd, "/admin") == 0) {
        if (strcmp(arg1, ADMIN_PASSWORD) == 0) {
            clients[idx].is_admin = 1;
            send_to_fd(clients[idx].fd, "[系統] 你已取得管理員權限。\n");
        } else {
            send_to_fd(clients[idx].fd, "[系統] 管理員密碼錯誤。\n");
        }
    } else if (strcmp(cmd, "/mute") == 0) {
        if (!clients[idx].is_admin) {
            send_to_fd(clients[idx].fd, "[系統] 權限不足，只有管理員可以禁言。\n");
            return;
        }
        if (arg1[0] == '\0' || arg2[0] == '\0') {
            send_to_fd(clients[idx].fd, "用法：/mute 名稱 秒數 或 /mute 名稱 forever\n");
            return;
        }
        int target = find_client_index_by_name(arg1);
        if (target < 0) {
            send_to_fd(clients[idx].fd, "[系統] 找不到該使用者。\n");
            return;
        }
        if (strcmp(arg2, "forever") == 0) {
            clients[target].mute_until = -1;
        } else {
            int seconds = atoi(arg2);
            if (seconds <= 0) {
                send_to_fd(clients[idx].fd, "[系統] 禁言秒數必須大於 0。\n");
                return;
            }
            clients[target].mute_until = time(NULL) + seconds;
        }
        char msg[BUFFER_SIZE];
        snprintf(msg, sizeof(msg), "[系統] %s 已被管理員禁言：%s\n", clients[target].name, arg2);
        printf("%s", msg);
        save_history(msg);
        broadcast_message(msg);
    } else if (strcmp(cmd, "/unmute") == 0) {
        if (!clients[idx].is_admin) {
            send_to_fd(clients[idx].fd, "[系統] 權限不足，只有管理員可以解除禁言。\n");
            return;
        }
        int target = find_client_index_by_name(arg1);
        if (target < 0) {
            send_to_fd(clients[idx].fd, "[系統] 找不到該使用者。\n");
            return;
        }
        clients[target].mute_until = 0;
        char msg[BUFFER_SIZE];
        snprintf(msg, sizeof(msg), "[系統] %s 已被解除禁言。\n", clients[target].name);
        printf("%s", msg);
        save_history(msg);
        broadcast_message(msg);
    } else if (strcmp(cmd, "/kick") == 0) {
        if (!clients[idx].is_admin) {
            send_to_fd(clients[idx].fd, "[系統] 權限不足，只有管理員可以踢人。\n");
            return;
        }
        int target = find_client_index_by_name(arg1);
        if (target < 0) {
            send_to_fd(clients[idx].fd, "[系統] 找不到該使用者。\n");
            return;
        }
        char msg[BUFFER_SIZE];
        snprintf(msg, sizeof(msg), "[系統] %s 已被管理員踢出聊天室。\n", clients[target].name);
        send_to_fd(clients[target].fd, "[系統] 你已被管理員踢出聊天室。\n");
        printf("%s", msg);
        save_history(msg);
        close(clients[target].fd);
        clients[target].fd = 0;
        clients[target].name[0] = '\0';
        clients[target].is_admin = 0;
        clients[target].mute_until = 0;
        broadcast_message(msg);
    } else {
        send_to_fd(clients[idx].fd, "[系統] 未知指令，輸入 /help 查看指令。\n");
    }
}

int is_muted(int idx) {
    if (clients[idx].mute_until == 0) return 0;
    if (clients[idx].mute_until == -1) return 1;
    if (time(NULL) < clients[idx].mute_until) return 1;
    clients[idx].mute_until = 0;
    send_to_fd(clients[idx].fd, "[系統] 你的禁言時間已結束。\n");
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int server_fd, new_socket, max_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    memset(clients, 0, sizeof(clients));

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        return 1;
    }

    printf("多人管理聊天室 Server 已啟動，Port: %s\n", argv[1]);
    printf("Server 可直接輸入公告，輸入 exit 關閉。\n");

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        max_fd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd > 0) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > max_fd) max_fd = clients[i].fd;
            }
        }

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select failed");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) continue;
            trim_newline(buffer);
            if (strcmp(buffer, "exit") == 0) {
                broadcast_message("[系統] Server 即將關閉。\n");
                break;
            }
            char msg[BUFFER_SIZE + 64];
            snprintf(msg, sizeof(msg), "[Server公告] %s\n", buffer);
            printf("%s", msg);
            save_history(msg);
            broadcast_message(msg);
        }

        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
            if (new_socket < 0) continue;

            int added = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = new_socket;
                    snprintf(clients[i].name, NAME_SIZE, "Client%d", new_socket);
                    clients[i].is_admin = 0;
                    clients[i].mute_until = 0;
                    added = 1;
                    break;
                }
            }

            if (!added) {
                send_to_fd(new_socket, "[系統] 聊天室人數已滿。\n");
                close(new_socket);
                continue;
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = clients[i].fd;
            if (fd > 0 && FD_ISSET(fd, &readfds)) {
                memset(buffer, 0, BUFFER_SIZE);
                int bytes = recv(fd, buffer, BUFFER_SIZE - 1, 0);

                if (bytes <= 0) {
                    remove_client(i);
                    continue;
                }

                buffer[bytes] = '\0';
                trim_newline(buffer);

                if (strncmp(buffer, "NAME:", 5) == 0) {
                    char newname[NAME_SIZE];
                    strncpy(newname, buffer + 5, NAME_SIZE - 1);
                    newname[NAME_SIZE - 1] = '\0';
                    if (strlen(newname) == 0 || find_client_index_by_name(newname) >= 0) {
                        snprintf(newname, NAME_SIZE, "Client%d", fd);
                    }
                    strncpy(clients[i].name, newname, NAME_SIZE - 1);
                    clients[i].name[NAME_SIZE - 1] = '\0';

                    char msg[BUFFER_SIZE];
                    snprintf(msg, sizeof(msg), "[系統] %s 加入聊天室。\n", clients[i].name);
                    printf("新使用者加入：%s:%d，名稱：%s\n",
                           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), clients[i].name);
                    save_history(msg);
                    broadcast_message(msg);
                    show_help(fd);
                    continue;
                }

                if (strcmp(buffer, "exit") == 0) {
                    remove_client(i);
                    continue;
                }

                if (buffer[0] == '/') {
                    handle_command(i, buffer);
                    continue;
                }

                if (is_muted(i)) {
                    send_to_fd(fd, "[系統] 你目前被禁言，不能發送訊息，但仍可觀看聊天室。\n");
                    continue;
                }

                char msg[BUFFER_SIZE + NAME_SIZE + 32];
                snprintf(msg, sizeof(msg), "%s: %s\n", clients[i].name, buffer);
                printf("%s", msg);
                save_history(msg);
                broadcast_message(msg);
            }
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0) close(clients[i].fd);
    }
    close(server_fd);
    return 0;
}
