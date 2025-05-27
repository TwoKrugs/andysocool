#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define SERVER_IP "172.31.1.44"  // 可改成伺服器 IP
#define SERVER_PORT 65520

int sock;

void* receive_messages(void* arg) {
    char buffer[1024];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            printf("\nDisconnected from server.\n");
            close(sock);
            exit(0);
        }
        printf("\r%s", buffer);  // 顯示訊息，覆蓋輸入列
        printf(": ");            // 重印提示符
        fflush(stdout);
    }
    return NULL;
}

int main() {
    struct sockaddr_in server_addr;
    char buffer[1024];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket ERROR: ");
        close(sock);
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect ERROR: ");
        close(sock);
        return 1;
    }

    printf("Connected to server.\n");

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, NULL);

    while (1) {
        printf(": ");
        fgets(buffer, sizeof(buffer), stdin);

        if (strcmp(buffer, "/exit\n") == 0) {
            send(sock, buffer, strlen(buffer), 0);  // 通知 server 離開
            break;
        }

        send(sock, buffer, strlen(buffer), 0);
    }

    close(sock);
    return 0;
}
