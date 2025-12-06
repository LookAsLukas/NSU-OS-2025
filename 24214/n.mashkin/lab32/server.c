#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/async_socket"

int listen_fd;
int *client_fds = NULL;
int max_clients = 1;
int num_clients = 0;

void sigio_handler(int sig) {
    int new_fd;

    while ((new_fd = accept(listen_fd, NULL, NULL)) != -1) {
        if (num_clients == max_clients) {
            max_clients *= 2;
            client_fds = realloc(client_fds, max_clients * sizeof(int));
            if (client_fds == NULL) {
                perror("realloc");
                close(listen_fd);
                free(client_fds);
                exit(1);
            }
        }
        client_fds[num_clients++] = new_fd;
        fcntl(new_fd, F_SETFL, O_NONBLOCK | O_ASYNC);
        fcntl(new_fd, F_SETOWN, getpid());
    }

    for (int i = 0; i < num_clients; i++) {
        char buffer[4096];
        ssize_t n;
        while ((n = read(client_fds[i], buffer, 4096)) > 0) {
            for (int j = 0; j < n; j++) {
                if (buffer[j] >= 'a' && buffer[j] <= 'z') {
                    buffer[j] -= 32;
                }
            }
            
            if (write(STDOUT_FILENO, buffer, n) != n) {
                perror("write");
                close(listen_fd);
                free(client_fds);
                exit(1);
            }
        }
        if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            close(client_fds[i]);
            client_fds[i] = client_fds[--num_clients];
            i--;
        }
    }
}

int main() {
    struct sockaddr_un addr;
    struct sigaction sa;

    listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    unlink(SOCKET_PATH);
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(listen_fd);
        exit(1);
    }

    if (listen(listen_fd, 0) == -1) {
        perror("listen");
        close(listen_fd);
        exit(1);
    }

    client_fds = malloc(sizeof(int));
    sa.sa_handler = sigio_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGIO, &sa, NULL) == -1) {
        perror("sigaction");
        close(listen_fd);
        free(client_fds);
        exit(1);
    }

    fcntl(listen_fd, F_SETFL, O_NONBLOCK | O_ASYNC);
    fcntl(listen_fd, F_SETOWN, getpid());

    printf("Server listening on %s\n", SOCKET_PATH);

    while (1) {
        pause();
    }

    close(listen_fd);
    free(client_fds);
    return 0;
}
