#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENTS 10

volatile sig_atomic_t wasSigHup = 0;

void sigHupHandler(int sig) {
    wasSigHup = 1;
}

int main() {
    int server_fd, new_socket, max_fd;
    int client_sockets[MAX_CLIENTS] = {0};
    fd_set fds;
    struct sigaction sa;
    sigset_t blockedMask, origMask;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Создание сокета
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Настройка адреса
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // Настройка обработчика сигналов
    sigaction(SIGHUP,NULL, &sa);
    sa.sa_handler = sigHupHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);

    // Блокировка SIGHUP для безопасной проверки в pselect
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK,blockedMask, &origMask);

    while (1) {
        FD_ZERO(&fds);
        FD_SET(server_fd, &fds);
        max_fd = server_fd;
        
        // Добавление клиентских сокетов в набор
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0) {
                FD_SET(client_sockets[i], &fds);
                if (client_sockets[i] > max_fd) {
                    max_fd = client_sockets[i];
                }
            }
        }

        // Безопасный вызов pselect с разблокировкой SIGHUP
        if (pselect(max_fd + 1, &fds, NULL, NULL, NULL, &origMask) == -1) {
            if (errno == EINTR) {
                if (wasSigHup) {
                    printf("Received SIGHUP signal\n");
                    wasSigHup = 0;
                }
                continue;
            } else {
                perror("pselect error");
                break;
            }
        }

        // Обработка нового подключения на основном сокете
        if (FD_ISSET(server_fd, &fds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                                   (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                continue;
            }

            printf("New connection accepted\n");
            // Закрытие всех существующих соединений 
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] > 0) {
                    close(client_sockets[i]);
                    client_sockets[i] = 0;
                    printf("Closed existing connection\n");
                }
            }
            // Добавление нового клиента
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        // Обработка данных от установленных соединений
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = client_sockets[i];
            if (fd > 0 && FD_ISSET(fd, &fds)) {
                char buffer[1024] = {0};
                int valread = read(fd, buffer, sizeof(buffer));
                if (valread > 0) {
                    printf("Received %d bytes from client\n", valread);
                } else {
                    close(fd);
                    client_sockets[i] = 0;
                    printf("Client disconnected\n");
                }
            }
        }
    }

    // Очистка
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] > 0) {
            close(client_sockets[i]);
        }
    }
    close(server_fd);
    return 0;
}
