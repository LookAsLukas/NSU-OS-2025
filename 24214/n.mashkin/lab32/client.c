#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/async_socket"

int main() {
    int sock_fd;
    struct sockaddr_un addr;

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock_fd);
        exit(1);
    }

    char buffer[4096];
    ssize_t n;
    while ((n = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
        if (write(sock_fd, buffer, n) != n) {
            perror("write");
            close(sock_fd);
            break;
        }
    }
    if (n == -1) {
        perror("read");
        close(sock_fd);
    }

    close(sock_fd);
    return 0;
}
