#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define WINDOW_SIZE 5
#define PACKET_SIZE 512
#define LOSS_PROBABILITY 0.2

typedef struct {
    int packet_no;
    int packet_size;
    char data[PACKET_SIZE];
} Packet;

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int window_size;
    Packet packet;
    int ack;
    int base = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Initialize server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // Receive window size
    recvfrom(sockfd, &window_size, sizeof(int), 0, (struct sockaddr*)&client_addr, &client_len);

    srand(time(NULL));

    while (1) {
        // Receive packet
        recvfrom(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr*)&client_addr, &client_len);

        printf("RECEIVE PACKET %d: ", packet.packet_no);

        // Simulate packet loss
        if ((double)rand() / RAND_MAX < LOSS_PROBABILITY) {
            printf("DROP: BASE %d\n", base);
            continue;
        }

        if (packet.packet_no == base) {
            // Packet received in order, copy data to the file
            FILE *out_file = fopen("out.txt", "ab");
            fwrite(packet.data, 1, packet.packet_size, out_file);
            fclose(out_file);
            ack = base;
            base++;
        } else {
            ack = base - 1;
        }

        printf("ACCEPT: BASE %d\n", base);

        // Send ACK
        sendto(sockfd, &ack, sizeof(int), 0, (struct sockaddr*)&client_addr, client_len);
        printf("SEND ACK %d\n", ack);
    }

    close(sockfd);

    return 0;
}