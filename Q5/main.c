#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "reactor.h"

void handleClient(int fd) {
    char buffer[1024];
    ssize_t bytes = read(fd, buffer, sizeof(buffer)-1);
    
    if (bytes <= 0) {
        close(fd);
        printf("Client disconnected\n");
        return;
    }
    
    buffer[bytes] = '\0';
    printf("Received: %s", buffer);
    write(fd, buffer, bytes);
}

void handleNewConnection(int fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(fd, (struct sockaddr*)&client_addr, &addr_len);
    
    Reactor* reactor = getCurrentReactor();
    addFd(reactor, client_fd, handleClient);
    printf("New client connected\n");
}

int main() {
    Reactor* reactor = createReactor();
    if (!reactor) {
        perror("Failed to create reactor");
        return 1;
    }
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(9034)
    };
    
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);
    
    addFd(reactor, server_fd, handleNewConnection);
    
    printf("Reactor running... Press Enter to stop\n");
    getchar();
    
    stopReactor(reactor);
    close(server_fd);
    return 0;
}