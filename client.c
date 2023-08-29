#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define WINDOW_SIZE 5
#define PACKET_SIZE 512
#define RETRANSMISSION_TIMEOUT 2

typedef struct {
    int packet_no;
    int packet_size;
    char data[PACKET_SIZE];
} Packet;

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    FILE *file;
    int total_packets;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Initialize server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Establish connection
    connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // Send window size
    int window_size = WINDOW_SIZE;
    send(sockfd, &window_size, sizeof(int), 0);

    // Open and read file
    file = fopen("file.txt", "rb");
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Calculate total packets
    total_packets = (file_size + PACKET_SIZE - 1) / PACKET_SIZE;
    //printf("%d", total_packets);

    int base = 0;
    int next_seq_num = 0;
    int ack;
    int ack_received[total_packets];
    memset(ack_received, 0, sizeof(ack_received));

    // Start sending packets
    while (base < total_packets) {
        // Send packets within window
        while (next_seq_num < base + window_size && next_seq_num < total_packets) {
            Packet packet;
            packet.packet_no = next_seq_num;
            packet.packet_size = fread(packet.data, 1, PACKET_SIZE, file);

            // Send the packet
            send(sockfd, &packet, sizeof(Packet), 0);
            printf("SEND PACKET %d: BASE %d\n", packet.packet_no, base);

            next_seq_num++;
        }

        // Timeout handling
        for (int i = base; i < next_seq_num; i++) {
            if (!ack_received[i]) {
                printf("TIMEOUT %d\n", i);
                next_seq_num = i;  // Reset next_seq_num to re-send packets starting from i
                break;
            }
        }

        // Receive ACKs
        while (recv(sockfd, &ack, sizeof(int), MSG_DONTWAIT) > 0) {
            printf("RECEIVE ACK %d: BASE %d\n", ack, base);
            if (ack >= base && ack < base + window_size) {
                ack_received[ack] = 1;
                while (ack_received[base]) {
                    base++;
                }
            }
        }
    }

    // Close connection and file
    close(sockfd);
    fclose(file);

    return 0;
}