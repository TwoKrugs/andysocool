// multi_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 12345
#define MAX_CLIENTS 10

typedef struct {
    int socket;
    struct sockaddr_in addr;
} client_info;

client_info clients[MAX_CLIENTS];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(const char *msg, int sender_fd) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0 && clients[i].socket != sender_fd) {
            send(clients[i].socket, msg, strlen(msg), 0);
        }
    }
    pthread_mutex_unlock(&lock);
}

void* handle_client(void* arg) {
    client_info *client = (client_info*)arg;
    int client_fd = client->socket;
    char buffer[1024], msg[1100];
    char client_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &(client->addr.sin_addr), client_ip, INET_ADDRSTRLEN);

    snprintf(msg, sizeof(msg), "[%s] joined the chat.\n", client_ip);
    printf("%s", msg);
    broadcast_message(msg, client_fd);

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0 || strcmp(buffer, "/exit\n") == 0) {
            snprintf(msg, sizeof(msg), "[%s] left the chat.\n", client_ip);
            printf("%s", msg);
            broadcast_message(msg, client_fd);

            close(client_fd);
            pthread_mutex_lock(&lock);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket == client_fd) {
                    clients[i].socket = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&lock);
            free(client);
            break;
        }

        snprintf(msg, sizeof(msg), "[%s] %s", client_ip, buffer);
        printf("%s", msg);
        broadcast_message(msg, client_fd);
    }

    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    memset(clients, 0, sizeof(clients));

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, MAX_CLIENTS);
    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        pthread_mutex_lock(&lock);
        int stored = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == 0) {
                clients[i].socket = client_fd;
                clients[i].addr = client_addr;
                stored = 1;
                break;
            }
        }
        pthread_mutex_unlock(&lock);

        if (!stored) {
            printf("Max clients reached. Connection rejected.\n");
            close(client_fd);
            continue;
        }

        client_info *new_client = malloc(sizeof(client_info));
        *new_client = (client_info){client_fd, client_addr};

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, new_client);
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}
