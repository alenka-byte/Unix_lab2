#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 12345
#define MAX_CLIENTS 1

volatile sig_atomic_t wasSigHup = 0;

void sigHupHandler(int sig) {
    wasSigHup = 1;
}

int main() {
    int serverSocket, clientSocket;
    // Создание сокета
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Настройка адреса сервера
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    // Привязка сокета к адресу
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("ошибка bind");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Прослушивание сокета
    if (listen(serverSocket, MAX_CLIENTS) < 0) {
        perror("ошибка listen");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }
    printf("сервер запущен на порту 12345. Ожидание соединений\n");

    // Регистрация обработчика сигнала
    struct sigaction sa;
    sigaction(SIGHUP, NULL, &sa);
    sa.sa_handler = sigHupHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);

    // Блокировка сигнала
    sigset_t blockedMask, origMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

    // Основной цикл приложения
    fd_set fds;
    int maxFd=serverSocket;
    while (true) {
        FD_ZERO(&fds);
        FD_SET(serverSocket, &fds);
        maxFd = serverSocket;

        // Добавление клиентского сокета, если он есть
        if (clientSocket > 0) {
			FD_SET(clientSocket, &fds);
            if (clientSocket > maxFd)
                maxFd = clientSocket;
        }

        // Вызов pselect()
        int ready = pselect(maxFd + 1, &fds, NULL, NULL, NULL, &origMask);
        if (ready == -1) {
            if (errno == EINTR)
			{
				printf("Получен сигнал\n");
				if (wasSigHup) {
					wasSigHup = 0;
					printf("Обработан сигнал SIGHUP\n");
				}
				continue;
			}
			else 
			{
				break;
			}
        }
     
            // Обработка нового соединения
            if (FD_ISSET(serverSocket, &fds)) {
                clientSocket = accept(serverSocket, NULL, NULL);
               if (clientSocket != -1) {
				printf("Новое соединение: %d\n", clientSocket);
               }
            }

            // Обработка данных от клиента
            if (clientSocket > 0 && FD_ISSET(clientSocket, &fds)) {
                char buffer[1024];
                ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer));
                if (bytesRead > 0) {
                    printf("получил %zd байтов\n", bytesRead);
                } else if (bytesRead == 0) {
                    printf("Client disconnected\n");
                    close(clientSocket);
                    clientSocket = -1;
                } else {
                    perror("read");
                    close(clientSocket);
                    clientSocket = -1;
                }
            }
        }
    

    close(serverSocket);
    return 0;
}
