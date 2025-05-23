#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    inet_pton(AF_INET, "chat-server", &server_addr.sin_addr);

    connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Connection successful.\n");

    while (1) {
        printf(": ");
        fgets(buffer, sizeof(buffer), stdin);
        send(sock, buffer, strlen(buffer), 0);

        memset(buffer, 0, sizeof(buffer));
        int recv_bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (recv_bytes == 0) {
            printf("Server is unconnected.\n");
            break;
        }

        printf("Server: %s", buffer);
    }

    close(sock);
    return 0;
}
