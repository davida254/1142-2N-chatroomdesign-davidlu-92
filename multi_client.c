#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024

void trim_newline(char *s) {
    s[strcspn(s, "\r\n")] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <server_ip> <port> <name>\n", argv[0]);
        printf("Example: %s 127.0.0.1 8888 David\n", argv[0]);
        return 1;
    }

    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("invalid address");
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        return 1;
    }

    char name_msg[BUFFER_SIZE];
    snprintf(name_msg, sizeof(name_msg), "NAME:%s", argv[3]);
    send(sockfd, name_msg, strlen(name_msg), 0);

    printf("已連線到聊天室，名稱：%s\n", argv[3]);
    printf("輸入訊息後按 Enter。輸入 /help 查看指令，輸入 exit 離開。\n");

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);

        int max_fd = sockfd;
        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select failed");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) continue;
            trim_newline(buffer);
            if (strlen(buffer) == 0) continue;

            send(sockfd, buffer, strlen(buffer), 0);
            if (strcmp(buffer, "exit") == 0) break;
        }

        if (FD_ISSET(sockfd, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (bytes <= 0) {
                printf("Server 已關閉或連線中斷。\n");
                break;
            }
            buffer[bytes] = '\0';
            printf("%s", buffer);
        }
    }

    close(sockfd);
    return 0;
}
