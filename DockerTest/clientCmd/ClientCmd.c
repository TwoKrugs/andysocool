#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <pthread.h>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")
//172.31.1.44 andyboss ip
//172.31.1.63 jon ip
#define IP "172.31.1.44"
#define PORT 65520
#define BUF_SIZE 2048

int sock;
char InputBuffer[BUF_SIZE];
int InputSizeNow = 0;

void* receive_handler(void* arg) {
    char buffer[BUF_SIZE];
    int length = 0;
    while (1) {
        memset(buffer, 0, BUF_SIZE);
        int bytes = recv(sock, buffer, BUF_SIZE, 0);
        if (bytes <= 0) {
            printf("\nDisconnected from server.\n");
            closesocket(sock);
            WSACleanup();
            exit(0);
        }

        const char *start = strstr(buffer, "LEN=");
        if (start) {
            char len_str[5] = {0};
            strncpy(len_str, start + 4, 4);
            length = atoi(len_str);
            const char *content = strchr(start, '|');
            if (content) content++;

            if (length >= 0 && content) {
                printf("\r%*s\r", InputSizeNow + 2, " ");
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
            printf("%.*s", InputSizeNow, InputBuffer);
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
    Sleep(10);
    send(sock, buffer, size, 0);
    free(buffer);
    printf("[Image sent to server]\n");
}

void read_input_realtime(char* buffer, size_t size) {
    InputSizeNow = 0;
    while (InputSizeNow < size - 1) {
        char ch = _getch();

        if (ch == '\b') {
            if (InputSizeNow > 0) {
                InputSizeNow--;
                printf("\b \b");
            }
            continue;
        }

        putchar(ch);
        buffer[InputSizeNow++] = ch;

        if (ch == '\r') {
            putchar('\n');
            break;
        }
    }

    buffer[InputSizeNow] = '\0';
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    struct sockaddr_in server_addr;
    struct addrinfo hints, *res;
    char name[100];
    char ip_input[100];
    char port_input[10];
    int port;

    printf("Enter server IP: ");
    fgets(ip_input, sizeof(ip_input), stdin);
    ip_input[strcspn(ip_input, "\n")] = 0;

    //-------------test case-------------
    if (ip_input[0] == 0){
      memset(ip_input, 0, 100);
      strcpy(ip_input, IP);
    }
    //-----------------------------------

    printf("Enter server Port: ");
    fgets(port_input, sizeof(port_input), stdin);

    //-------------test case-------------
    if (port_input[0] = '\n'){
      port = PORT;
    } else {
      port = atoi(port_input);
    }
    //-----------------------------------

    printf("Enter your name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("socket failed\n");
        return 1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(ip_input, NULL, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerrorA(err));
        return 1;
    }

    server_addr = *(struct sockaddr_in *)res->ai_addr;
    server_addr.sin_port = htons(port);
    freeaddrinfo(res);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect ERROR");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    send(sock, name, strlen(name), 0);
    printf("Connected to server as %s.\n", name);

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_handler, NULL);

    while (1) {
        printf(": ");
        read_input_realtime(InputBuffer, BUF_SIZE);
        // fgets(InputBuffer, BUF_SIZE, stdin);

        if (strncmp(InputBuffer, "/img ", 5) == 0) {
            InputBuffer[strcspn(InputBuffer, "\n")] = 0;
            send_image(InputBuffer + 5);
            continue;
        } else if (strcmp(InputBuffer, "/exit\n") == 0) {
            send(sock, InputBuffer, strlen(InputBuffer), 0);
            break;
        }

        send(sock, InputBuffer, strlen(InputBuffer) - 1, 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
