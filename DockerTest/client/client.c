// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>       // 新增：用來解析域名
#include <sys/stat.h>

//*/
#define SERVER_HOST "turntable.proxy.rlwy.net"
#define PORT 58503
/*/
#define SERVER_HOST "172.31.1.44"
#define PORT 65520
//*/
#define BUF_SIZE 2048

int sock;

void* receive_handler(void* arg) {
    char buffer[BUF_SIZE];
    int length = 0;
    while (1) {
        memset(buffer, 0, BUF_SIZE);
        int bytes = recv(sock, buffer, BUF_SIZE, 0);
        if (bytes <= 0) {
            printf("\nDisconnected from server.\n");
            close(sock);
            exit(0);
        }

        // 嘗試解析長度
        const char *start = strstr(buffer, "LEN=");
        if (start) {
          char len_str[5] = {0}; // "0012"
          strncpy(len_str, start + 4, 4);
          length = atoi(len_str);

          // 找訊息開始點
          const char *content = strchr(start, '|');
          if (content) content++;

          if (length >= 0 && content) {
            printf("\r");
            length -= printf("%s", content);
            fflush(stdout);
          }
      } else {
          if (length > 0) {
            length -= printf("%s", buffer);
            fflush(stdout);
          }
        }

        if (length <= 0) {
          printf(": ");
          fflush(stdout);
        }
    }
    return NULL;
}

void send_image(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("Failed to open image");
        return;
    }
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buffer = malloc(size);
    if (!buffer) {
        perror("malloc failed");
        fclose(f);
        return;
    }
    fread(buffer, 1, size, f);
    fclose(f);

    char header[64];
    sprintf(header, "IMAGE:%d", size);
    send(sock, header, strlen(header), 0);
    usleep(10000);
    send(sock, buffer, size, 0);
    free(buffer);
    printf("[Image sent to server]\n");
}

int main() {
    struct sockaddr_in server_addr;
    struct addrinfo hints, *res;
    char buffer[BUF_SIZE];
    char name[100];

    printf("Enter your name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket ERROR");
        return 1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(SERVER_HOST, NULL, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return 1;
    }

    server_addr = *(struct sockaddr_in *)res->ai_addr;
    server_addr.sin_port = htons(PORT);

    freeaddrinfo(res);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect ERROR");
        close(sock);
        return 1;
    }

    send(sock, name, strlen(name), 0);  // 傳送名稱給 server

    printf("Connected to server as %s.\n", name);

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_handler, NULL);

    while (1) {
        printf(": ");
        fgets(buffer, BUF_SIZE, stdin);

        if (strncmp(buffer, "/img ", 5) == 0) {
            buffer[strcspn(buffer, "\n")] = 0;
            send_image(buffer + 5);
            continue;
        } else if (strcmp(buffer, "/exit\n") == 0) {
            send(sock, buffer, strlen(buffer), 0);
            break;
        }

        send(sock, buffer, strlen(buffer) - 1, 0);
    }

    close(sock);
    return 0;
}
