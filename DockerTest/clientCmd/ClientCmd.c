#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <pthread.h>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")
// 172.31.1.44 andyboss ip
// 172.31.1.63 jon ip
#define IP        "172.31.1.44"
#define PORT      65520
#define BUF_SIZE  2048

int   gSock;
char  gInputBuffer[BUF_SIZE];
int   gInputSizeNow = 0;

void *
receive_handler (
  void  *arg
  )
{
  char  Buffer[BUF_SIZE];
  int   Length = 0;

  while (1) {
    memset (Buffer, 0, BUF_SIZE);
    int  Bytes = recv (gSock, Buffer, BUF_SIZE, 0);
    if (Bytes <= 0) {
      printf ("\nDisconnected from server.\n");
      closesocket (gSock);
      WSACleanup ();
      exit (0);
    }

    const char  *Start = strstr (Buffer, "LEN=");
    if (Start) {
      char  LenStr[5] = { 0 };
      strncpy (LenStr, Start + 4, 4);
      Length = atoi (LenStr);
      const char  *Content = strchr (Start, '|');
      if (Content) {
        Content++;
      }

      if ((Length >= 0) && Content) {
        printf ("\r%*s\r", gInputSizeNow + 2, " ");
        Length -= printf ("%s", Content);
        fflush (stdout);
      }
    } else {
      if (Length > 0) {
        Length -= printf ("%s", Buffer);
        fflush (stdout);
      }
    }

    if (Length <= 0) {
      printf (": ");
      printf ("%.*s", gInputSizeNow, gInputBuffer);
      fflush (stdout);
    }
  }

  return NULL;
}

void
send_image (
  const char  *Path
  )
{
  FILE  *f = fopen (Path, "rb");

  if (!f) {
    perror ("Failed to open image");
    return;
  }

  fseek (f, 0, SEEK_END);
  int  ImageSize = ftell (f);
  fseek (f, 0, SEEK_SET);
  char  *Buffer = malloc (ImageSize);
  if (!Buffer) {
    perror ("malloc failed");
    fclose (f);
    return;
  }

  fread (Buffer, 1, ImageSize, f);
  fclose (f);

  char  header[64];
  sprintf (header, "IMAGE:%d", ImageSize);
  send (gSock, header, strlen (header), 0);
  Sleep (10);
  send (gSock, Buffer, ImageSize, 0);
  free (Buffer);
  printf ("[Image sent to server]\n");
}

void
read_input_realtime (
  char    *Buffer,
  size_t  MaxInputSize
  )
{
  gInputSizeNow = 0;
  while (gInputSizeNow < MaxInputSize - 1) {
    char  InputChar = _getch ();

    if (InputChar == '\b') {
      if (gInputSizeNow > 0) {
        gInputSizeNow--;
        printf ("\b \b");
      }

      continue;
    }

    putchar (InputChar);
    Buffer[gInputSizeNow++] = InputChar;

    if (InputChar == '\r') {
      putchar ('\n');
      break;
    }
  }

  Buffer[gInputSizeNow] = '\0';
}

int
main (
  )
{
  WSADATA  Wsa;

  if (WSAStartup (MAKEWORD (2, 2), &Wsa) != 0) {
    printf ("WSAStartup failed\n");
    return 1;
  }

  struct sockaddr_in  ServerAddr;
  struct addrinfo     Hints, *Res;
  char                Name[100];
  char                IpInput[100];
  char                PortInput[10];
  int                 Port;

  printf ("Enter server IP: ");
  fgets (IpInput, sizeof (IpInput), stdin);
  IpInput[strcspn (IpInput, "\n")] = 0;

  // -------------test case-------------
  if (IpInput[0] == 0) {
    memset (IpInput, 0, 100);
    strcpy (IpInput, IP);
  }

  // -----------------------------------

  printf ("Enter server Port: ");
  fgets (PortInput, sizeof (PortInput), stdin);

  // -------------test case-------------
  if (PortInput[0] = '\n') {
    Port = PORT;
  } else {
    Port = atoi (PortInput);
  }

  // -----------------------------------

  printf ("Enter your Name: ");
  fgets (Name, sizeof (Name), stdin);
  Name[strcspn (Name, "\n")] = 0;

  gSock = socket (AF_INET, SOCK_STREAM, 0);
  if (gSock == INVALID_SOCKET) {
    printf ("socket failed\n");
    return 1;
  }

  memset (&Hints, 0, sizeof (Hints));
  Hints.ai_family   = AF_INET;
  Hints.ai_socktype = SOCK_STREAM;

  int  err = getaddrinfo (IpInput, NULL, &Hints, &Res);
  if (err != 0) {
    fprintf (stderr, "getaddrinfo: %s\n", gai_strerrorA (err));
    return 1;
  }

  ServerAddr          = *(struct sockaddr_in *)Res->ai_addr;
  ServerAddr.sin_port = htons (Port);
  freeaddrinfo (Res);

  if (connect (gSock, (struct sockaddr *)&ServerAddr, sizeof (ServerAddr)) < 0) {
    perror ("connect ERROR");
    closesocket (gSock);
    WSACleanup ();
    return 1;
  }

  send (gSock, Name, strlen (Name), 0);
  printf ("Connected to server as %s.\n", Name);

  pthread_t  recv_thread;
  pthread_create (&recv_thread, NULL, receive_handler, NULL);

  while (1) {
    printf (": ");
    read_input_realtime (gInputBuffer, BUF_SIZE);
    // fgets(gInputBuffer, BUF_SIZE, stdin);

    if (strncmp (gInputBuffer, "/img ", 5) == 0) {
      gInputBuffer[strcspn (gInputBuffer, "\n")] = 0;
      send_image (gInputBuffer + 5);
      continue;
    } else if (strcmp (gInputBuffer, "/exit\n") == 0) {
      send (gSock, gInputBuffer, strlen (gInputBuffer), 0);
      break;
    }

    send (gSock, gInputBuffer, strlen (gInputBuffer) - 1, 0);
  }

  closesocket (gSock);
  WSACleanup ();
  return 0;
}
