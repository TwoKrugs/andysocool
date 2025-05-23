#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1024];
    socklen_t addr_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(12345);

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 1);

    printf("Waiting for connection...\n");
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
    printf("Client is connected.\n");

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int recv_bytes = recv(client_fd, buffer, sizeof(buffer), 0);
        if (recv_bytes == 0) {
            printf("User is unconnected.\n");
            break;
        }

        printf("Client: %s", buffer);

        printf(": ");
        fgets(buffer, sizeof(buffer), stdin);
        send(client_fd, buffer, strlen(buffer), 0);
    }

    close(client_fd);
    close(server_fd);
    return 0;
}