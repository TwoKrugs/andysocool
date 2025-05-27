// // client.c
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <pthread.h>
// #include <arpa/inet.h>

// // #define IP "172.31.0.10"
// #define IP "172.31.1.44"
// // #define PORT 12345
// #define PORT 10247

// int sock;

// void* receive_messages(void* arg) {
//     char buffer[1024];
//     while (1) {
//         memset(buffer, 0, sizeof(buffer));
//         int bytes = recv(sock, buffer, sizeof(buffer), 0);
//         if (bytes <= 0) {
//             printf("Server disconnected.\n");
//             exit(0);
//         }
//         printf("\nServer: %s", buffer);
//         printf(": "); fflush(stdout);
//     }
//     return NULL;
// }

// int main() {
//     struct sockaddr_in server_addr;
//     char buffer[1024];

//     sock = socket(AF_INET, SOCK_STREAM, 0);
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(PORT);
//     inet_pton(AF_INET, IP, &server_addr.sin_addr);  // 改成伺服器 IP

//     connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
//     printf("Connected to server.\n");

//     pthread_t recv_thread;
//     pthread_create(&recv_thread, NULL, receive_messages, NULL);

//     while (1) {
//         printf(": ");
//         fgets(buffer, sizeof(buffer), stdin);
//         send(sock, buffer, strlen(buffer), 0);
//     }

//     close(sock);
//     return 0;
// }
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define SERVER_IP "172.31.1.63" // 請改成你 server 的 IP
#define PORT 2869
#define BUF_SIZE 2048

int sock;

void* receive_handler(void* arg) {
    char buffer[BUF_SIZE];
    while (1) {
        memset(buffer, 0, BUF_SIZE);
        int bytes = recv(sock, buffer, BUF_SIZE, 0);
        if (bytes <= 0) {
            printf("Server disconnected.\n");
            exit(0);
        }

        if (strncmp(buffer, "IMAGE:", 6) == 0) {
            int size = atoi(buffer + 6);
            char* img_data = malloc(size);
            int received = 0;
            while (received < size) {
                int r = recv(sock, img_data + received, size - received, 0);
                if (r <= 0) break;
                received += r;
            }
            FILE* f = fopen("received_from_server.jpg", "wb");
            fwrite(img_data, 1, size, f);
            fclose(f);
            free(img_data);
            printf("\n[Image received from server]\n");
            system("jp2a --width=80 received_from_server.jpg");
        } else {
            printf("\nServer: %s", buffer);
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
    send(sock, header, strlen(header), 0);
    usleep(10000);
    send(sock, buffer, size, 0);
    free(buffer);
    printf("[Image sent to server]\n");
}

int main() {
    struct sockaddr_in server_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Connected to server.\n");

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
            send(sock, buffer, strlen(buffer), 0);
        }
    }

    close(sock);
    return 0;
}
