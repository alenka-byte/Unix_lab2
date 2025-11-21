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
    fd_set read_fds;
    struct sigaction sa;
    sigset_t blocked_mask, empty_mask;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int active_client_count = 0;

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
    sa.sa_handler = sigHupHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Блокировка SIGHUP для безопасной проверки в pselect
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &blocked_mask, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    sigemptyset(&empty_mask);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;
        
        // Добавление клиентских сокетов в набор
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0) {
                FD_SET(client_sockets[i], &read_fds);
                if (client_sockets[i] > max_fd) {
                    max_fd = client_sockets[i];
                }
            }
        }

        // Безопасный вызов pselect с разблокировкой SIGHUP
        int activity = pselect(max_fd + 1, &read_fds, NULL, NULL, NULL, &empty_mask);
        
        if (activity < 0) {
            if (errno == EINTR) {
                // Проверка сигнала после возврата из pselect
                if (wasSigHup) {
                    printf("Received SIGHUP signal\n");
                    wasSigHup = 0;  // Безопасно - мы в основном потоке
                }
                continue;
            } else {
                perror("pselect error");
                break;
            }
        }

        // Обработка нового подключения
        if (FD_ISSET(server_fd, &read_fds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                                   (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                continue;
            }

            printf("New connection accepted\n");

            // Закрываем предыдущие соединения, оставляя только новое
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] > 0) {
                    close(client_sockets[i]);
                    client_sockets[i] = 0;
                }
            }
            client_sockets[0] = new_socket;
        }

        // Обработка данных от клиента
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0 && FD_ISSET(client_sockets[i], &read_fds)) {
                char buffer[1024] = {0};
                int valread = read(client_sockets[i], buffer, sizeof(buffer));
                if (valread > 0) {
                    printf("Received %d bytes from client\n", valread);
                } else {
                    close(client_sockets[i]);
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
