#include <vector>       
#include <signal.h>     
#include <sys/select.h> 
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h>     
#include <errno.h>      
#include <stdio.h>      
#include <stdlib.h>     


// Объявление обработчика сигнала
volatile sig_atomic_t wasSigHup = 0;
void sigHupHandler(int r)
{
	wasSigHup = 1;
}

int main() {
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


	// сокет
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	// Привязка к порту
	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(1255);  

	if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
		perror("Ошибка bind");
		exit(EXIT_FAILURE);
	}

	if (listen(serverSocket, SOMAXCONN) < 0) {
		perror("Ошибка listen");
		exit(EXIT_FAILURE);
	}

	printf("Сервер запущен на порту 1255. Ожидание соединений...\n");

	// Работа основного цикла
	std::vector<int> clients;
	int maxFd = serverSocket;
	fd_set fds; // Переменная для хранения всех дескрипторов

	while (true)
	{
		FD_ZERO(&fds);	// сбрасываем каждый раз обязательно
		FD_SET(serverSocket, &fds);
		maxFd = serverSocket;

		for (int clientSocket : clients) {
			FD_SET(clientSocket, &fds);
			if (clientSocket > maxFd) maxFd = clientSocket;
		}

		if (pselect(maxFd + 1, &fds, NULL, NULL, NULL, &origMask) == -1)
		{
			if (errno == EINTR)
			{
				printf("Получен сигнал SIGHUP\n");
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

		// some actions on the descriptor activity
		if (FD_ISSET(serverSocket, &fds)) {
			int clientSocket = accept(serverSocket, NULL, NULL);
			if (clientSocket != -1) {
				printf("Новое соединение: %d\n", clientSocket);

				// Оставляем одно соединение, остальные закрываем
				for (int oldSocket : clients) {
					printf("Оставляем одно соединение. Закрываем %d\n", oldSocket);
					close(oldSocket);
				}
				clients.clear();
				clients.push_back(clientSocket);
			}
		}

		// for the main socket and for every established connection
		for (auto clientIt = clients.begin(); clientIt != clients.end();) {
			int clientSocket = *clientIt;
			if (FD_ISSET(clientSocket, &fds)) {
				char buffer[1024];
				ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer));
				if (bytesRead <= 0) {
					printf("Соединение %d закрыто.\n", clientSocket);
					close(clientSocket);
					clientIt = clients.erase(clientIt);
				}
				else {
					printf("%zd байт от %d\n", bytesRead, clientSocket);
					++clientIt;
				}
			}
			else {
				++clientIt;
			}
		}

	}

	close(serverSocket);
	return 0;
}
