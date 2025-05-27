// // server.c
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <pthread.h>
// #include <arpa/inet.h>

// #define PORT 12345

// int client_fd;

// void* receive_messages(void* arg) {
//     char buffer[1024];
//     while (1) {
//         memset(buffer, 0, sizeof(buffer));
//         int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
//         if (bytes <= 0) {
//             printf("Client disconnected.\n");
//             exit(0);
//         }
//         printf("\nClient: %s", buffer);
//         printf(": "); fflush(stdout);
//     }
//     return NULL;
// }

// int main() {
//     int server_fd;
//     struct sockaddr_in server_addr, client_addr;
//     socklen_t addr_len = sizeof(client_addr);
//     char buffer[1024];

//     server_fd = socket(AF_INET, SOCK_STREAM, 0);

//     server_addr.sin_family = AF_INET;
//     server_addr.sin_addr.s_addr = INADDR_ANY;
//     server_addr.sin_port = htons(PORT);

//     bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
//     listen(server_fd, 1);
//     printf("Waiting for connection...\n");

//     client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
//     printf("Client connected.\n");

//     pthread_t recv_thread;
//     pthread_create(&recv_thread, NULL, receive_messages, NULL);

//     while (1) {
//         printf(": ");
//         fgets(buffer, sizeof(buffer), stdin);
//         send(client_fd, buffer, strlen(buffer), 0);
//     }

//     close(client_fd);
//     close(server_fd);
//     return 0;
// }
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 12345
#define BUF_SIZE 2048

int client_fd;

void* receive_handler(void* arg) {
    char buffer[BUF_SIZE];
    while (1) {
        memset(buffer, 0, BUF_SIZE);
        int bytes = recv(client_fd, buffer, BUF_SIZE, 0);
        if (bytes <= 0) {
            printf("Client disconnected.\n");
            exit(0);
        }

        if (strncmp(buffer, "IMAGE:", 6) == 0) {
            int size = atoi(buffer + 6);
            char* img_data = malloc(size);
            int received = 0;
            while (received < size) {
                int r = recv(client_fd, img_data + received, size - received, 0);
                if (r <= 0) break;
                received += r;
            }
            FILE* f = fopen("received_from_client.jpg", "wb");
            fwrite(img_data, 1, size, f);
            fclose(f);
            free(img_data);
            printf("\n[Image received from client]\n");
            system("jp2a --width=80 received_from_client.jpg");
        } else {
            printf("\nClient: %s", buffer);
        }
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
    send(client_fd, header, strlen(header), 0);
    usleep(10000);
    send(client_fd, buffer, size, 0);
    free(buffer);
    printf("[Image sent to client]\n");
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 1);
    printf("Waiting for connection...\n");
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
    printf("Client connected.\n");

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_handler, NULL);

    char buffer[BUF_SIZE];
    while (1) {
        printf(": ");
        fgets(buffer, BUF_SIZE, stdin);

        if (strncmp(buffer, "/img ", 5) == 0) {
            buffer[strcspn(buffer, "\n")] = 0;
            send_image(buffer + 5);
        } else {
            send(client_fd, buffer, strlen(buffer), 0);
        }
    }

    close(client_fd);
    close(server_fd);
    return 0;
}