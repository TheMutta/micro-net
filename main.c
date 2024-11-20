#define _GNU_SOURCE  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#define PORT 8080
#define BACKLOG 1000
#define SIZE 1024

void get_file_url(char *route, char *fileURL) {
	char *question = strrchr(route, '?');
	if (question)
		*question = '\0';

	if (route[strlen(route) - 1] == '/')
	{
		strcat(route, "index.html");
	}

	strcpy(fileURL, "htdocs");
	strcat(fileURL, route);

	const char *dot = strrchr(fileURL, '.');
	if (!dot || dot == fileURL)
	{
		strcat(fileURL, ".html");
	}
}

void getMimeType(char *file, char *mime)
{
  const char *dot = strrchr(file, '.');

  if (dot == NULL)
    strcpy(mime, "text/html");

  else if (strcmp(dot, ".html") == 0)
    strcpy(mime, "text/html");

  else if (strcmp(dot, ".css") == 0)
    strcpy(mime, "text/css");

  else if (strcmp(dot, ".js") == 0)
    strcpy(mime, "application/js");

  else if (strcmp(dot, ".jpg") == 0)
    strcpy(mime, "image/jpeg");

  else if (strcmp(dot, ".png") == 0)
    strcpy(mime, "image/png");

  else if (strcmp(dot, ".gif") == 0)
    strcpy(mime, "image/gif");

  else
    strcpy(mime, "text/html");
}

void getTimeString(char *buf)
{
  time_t now = time(0);
  struct tm tm = *gmtime(&now);
  strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);
}

int socket_handler() {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

	struct sockaddr_in sock;
	sock.sin_family = AF_INET;
	sock.sin_port = htons(PORT);
	sock.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	assert(bind(sockfd, (struct sockaddr*)&sock, sizeof(sock)) >= 0);
	assert(listen(sockfd, BACKLOG) >= 0);

	char hostBuffer[NI_MAXHOST], serviceBuffer[NI_MAXSERV];
	assert(getnameinfo((struct sockaddr *)&sock, sizeof(sock), hostBuffer,
                        sizeof(hostBuffer), serviceBuffer, sizeof(serviceBuffer), 0) == 0);

	printf("\nServer is listening on http://%s:%s/\n\n", hostBuffer, serviceBuffer);

	for (;;) {
		char *request = (char *)malloc(SIZE * sizeof(char));
		char method[10], route[100];

		int acceptfd = accept(sockfd, NULL, NULL);
		read(acceptfd, request, SIZE);

		sscanf(request, "%s %s", method, route);
		printf("%s %s", method, route);

		free(request);

		char file_url[100];

		get_file_url(route, file_url);

		FILE *file = fopen(file_url, "r");

		if (!file) {
			printf("\nFile not found\n");
			const char response[] = "HTTP/1.1 404 Not Found\r\n\n";
			send(acceptfd, response, sizeof(response), 0);
		} else {
			char resHeader[SIZE];

			char mimeType[32];
			getMimeType(file_url, mimeType);

			char timeBuf[100];
			getTimeString(timeBuf);

			sprintf(resHeader, "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: %s\r\n\n", timeBuf, mimeType);
			int headerSize = strlen(resHeader);

			printf(" %s", mimeType);

			fseek(file, 0, SEEK_END);
			long fsize = ftell(file);
			fseek(file, 0, SEEK_SET);

			char *resBuffer = (char *)malloc(fsize + headerSize);
			strcpy(resBuffer, resHeader);
			char *fileBuffer = resBuffer + headerSize;
			fread(fileBuffer, fsize, 1, file);

			send(acceptfd, resBuffer, fsize + headerSize, 0);
			free(resBuffer);
			fclose(file);
		}
		close(acceptfd);
	}

	close(sockfd);

	return EXIT_SUCCESS;
}

int main() {
	return socket_handler();
}
