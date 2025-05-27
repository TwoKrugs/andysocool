// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define SERVER_IP "172.31.1.44"
#define PORT 65520
#define BUF_SIZE 2048

int sock;

void* receive_handler(void* arg) {
    char buffer[BUF_SIZE];
    while (1) {
        memset(buffer, 0, BUF_SIZE);
        int bytes = recv(sock, buffer, BUF_SIZE, 0);
        if (bytes <= 0) {
            printf("\nDisconnected from server.\n");
            close(sock);
            exit(0);
        }
        printf("\r%s", buffer);
        printf(": "); fflush(stdout);
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
    char buffer[BUF_SIZE];
    char name[100];

    printf("Enter your name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket ERROR: ");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect ERROR: ");
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
        } else if (strcmp(buffer, "/exit\n") == 0) {
            send(sock, buffer, strlen(buffer), 0);
            break;
        }

        send(sock, buffer, strlen(buffer), 0);
    }

    close(sock);
    return 0;
}
