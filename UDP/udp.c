#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>          // read, write, close
#include <arpa/inet.h>       // inet_pton, htons 等
#include <sys/socket.h>      // socket(), bind(), connect() 等
#include <netinet/in.h>      // sockaddr_in 結構
#include <pthread.h>


#define BUF_SIZE 1024
#define PORT 8888


int sockfd;
struct sockaddr_in peer_addr;
socklen_t addr_len = sizeof(peer_addr);

void *send_loop(void *arg) {
    char buf[BUF_SIZE];
    while (1) {
        fgets(buf, BUF_SIZE, stdin);
        sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&peer_addr, addr_len);
    }
    return NULL;
}

void *recv_loop(void *arg) {
    char buf[BUF_SIZE];
    while (1) {
        int len = recvfrom(sockfd, buf, BUF_SIZE - 1, 0, NULL, NULL);
        if (len > 0) {
            buf[len] = '\0';
            printf("[對方] %s", buf);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("用法：%s <對方IP位址>\n", argv[0]);
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // 本機端綁定
    struct sockaddr_in self_addr;
    memset(&self_addr, 0, sizeof(self_addr));
    self_addr.sin_family = AF_INET;
    self_addr.sin_addr.s_addr = INADDR_ANY;
    self_addr.sin_port = htons(PORT);
    bind(sockfd, (struct sockaddr *)&self_addr, sizeof(self_addr));

    // 對方
    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, argv[1], &peer_addr.sin_addr);

    // 建立兩個執行緒
    pthread_t send_thread, recv_thread;
    pthread_create(&send_thread, NULL, send_loop, NULL);
    pthread_create(&recv_thread, NULL, recv_loop, NULL);

    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    close(sockfd);
    return 0;
}