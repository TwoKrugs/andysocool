#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <pthread.h>
#include <conio.h>
#include <fcntl.h>
#include <errno.h>

#pragma comment(lib, "ws2_32.lib")

#define IP        L"172.26.32.1"
#define PORT      65520
#define BUF_SIZE  2048
#define NAME_LEN  100

int      gSock;
wchar_t  gInputBuffer[BUF_SIZE];
int      gInputSizeNow = 0;
wchar_t  gChatTarget[NAME_LEN];

void wperror_errno(const wchar_t *prefix) {
    char *errmsg = strerror(errno);
    wchar_t werrmsg[512];
    mbstowcs(werrmsg, errmsg, 512);
    fwprintf(stderr, L"%ls: %ls\n", prefix, werrmsg);
}

boolean is_chinese(wchar_t ch) {
  return (
          (ch >= 0x4E00 && ch <= 0x9FFF) || // 常見中日韓漢字
          (ch >= 0x3400 && ch <= 0x4DBF) || // 擴展A區
          (ch >= 0x3000 && ch <= 0x303F) || // 中文標點符號
          (ch >= 0x3100 && ch <= 0x312F) || // 注音符號
          (ch >= 0x31A0 && ch <= 0x31BF)    // 注音擴展
          );
}

// 轉 UTF-8 傳送函式
int send_utf8(int sock, const wchar_t *input) {
    if (!input) return -1;

    // UTF-8 buffer 最大是 wchar_t 數量乘3
    char utf8buf[BUF_SIZE * 3] = {0};
    int len = WideCharToMultiByte(CP_UTF8, 0, input, -1, utf8buf, sizeof(utf8buf), NULL, NULL);
    if (len <= 0) {
        wprintf(L"[Error] Failed to convert wide char to UTF-8\n");
        return -1;
    }

    // 傳送時不含結尾 null 字元
    return send(sock, utf8buf, len - 1, 0);
}

void *receive_handler(void *arg) {
    char rawBuffer[BUF_SIZE] = {0};
    char *Content = NULL;
    wchar_t OutputBuffer[BUF_SIZE] = {0};
    int Length = 0;

    while (1) {
        int Bytes = recv(gSock, rawBuffer, BUF_SIZE - 1, 0);
        if (Bytes <= 0) {
            wprintf(L"\nDisconnected from server.\n");
            closesocket(gSock);
            WSACleanup();
            exit(0);
        }
        rawBuffer[Bytes] = '\0'; // 確保字串結尾

        char *Start = strstr(rawBuffer, "LEN=");
        if (Start) {
            char LenStr[5] = {0};
            char *endptr;
            strncpy(LenStr, Start + 4, 4);
            Length = strtol(LenStr, &endptr, 10);
            Content = strstr(Start, "|");
            if (Content) {
                Content++;
            }

            if ((Length >= 0) && Content) {
                wprintf(L"\r%*s\r", gInputSizeNow + 2, L" ");
                Length -= strlen(Content);
            }
        } else {
            if (Length > 0) {
                Content = rawBuffer;
                Length -= strlen(rawBuffer);
            }
        }

        int wLen = MultiByteToWideChar(CP_UTF8, 0, Content, -1, OutputBuffer, BUF_SIZE);
        if (wLen <= 0) {
            wprintf(L"[Error] Failed to convert UTF-8 to wide char\n");
            continue;
        }

        size_t SkipLen = 0;

        if (wcsncmp (OutputBuffer, L"/private", 8) == 0){
          wchar_t temp[NAME_LEN];
          wcscpy (temp, OutputBuffer + 8);
          const wchar_t *sep = wcschr(temp, '|');
          SkipLen = sep - temp; // 計算 | 前的長度
          memset (gChatTarget, 0, NAME_LEN);
          wcsncpy(gChatTarget, temp, SkipLen);
          SkipLen += 8;
        }

        wprintf(L"%s", OutputBuffer + SkipLen);
        fflush(stdout);
        if (Length <= 0) {
          wprintf(L"%s: ", gChatTarget);
          wprintf(L"%s", gInputBuffer);
          fflush(stdout);
        }
    }
    return NULL;
}

void send_image(const wchar_t *Path) {
  FILE *f = _wfopen(Path, L"rb");
  if (!f) {
    wperror_errno(L"Failed to open image");
    return;
  }

  fseek(f, 0, SEEK_END);
  int ImageSize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *Buffer = malloc(ImageSize);
  if (!Buffer) {
    wperror_errno(L"malloc failed");
    fclose(f);
    return;
  }

  fread(Buffer, 1, ImageSize, f);
  fclose(f);

  char header[64];
  sprintf(header, "IMAGE:%d", ImageSize);
  send(gSock, header, strlen(header), 0);
  Sleep(10);
  send(gSock, Buffer, ImageSize, 0);
  free(Buffer);

  wprintf(L"[Image sent to server]\n");
}

void read_input_realtime() {
  memset(gInputBuffer, 0, BUF_SIZE);
  gInputSizeNow = 0;
  int Position = 0;
  wchar_t ch;
  while (gInputSizeNow < BUF_SIZE - 1) {
    ch = _getwch();

    if (((int)ch == 0) || ((int)ch == 0xE0)) {
      
      int key = _getwch();
      if (key == 75) {
        
        if (Position > 0) {
          Position--;
          is_chinese(gInputBuffer[Position]) ? wprintf(L"\b\b") : wprintf(L"\b");
        }
      } else if (key == 77) {
        
         if (Position < gInputSizeNow) {
              wprintf(L"%lc", gInputBuffer[Position++]);
          }
      }
      continue;
    }

    if (ch == '\b') {
      if ((gInputSizeNow > 0) && (Position > 0)) {
        Position--;
        boolean IsChinese = is_chinese(gInputBuffer[Position]);
        IsChinese ? wprintf(L"\b\b  \b\b") : wprintf(L"\b \b");
        for (int i = Position; i < gInputSizeNow - 1; ++i) {
          gInputBuffer[i] = gInputBuffer[i + 1];
        }
        gInputSizeNow--;
        gInputBuffer[gInputSizeNow] = L'\0';
        for (int i = Position; i < gInputSizeNow; ++i) {
          wprintf(L"%lc", gInputBuffer[i]);
        }
        IsChinese ? wprintf(L"  \b\b") : wprintf(L" \b");
        for (int i = Position; i < gInputSizeNow; ++i) {
          is_chinese(gInputBuffer[i]) ? wprintf(L"\b\b") : wprintf(L"\b");
        }
      }
      continue;
    }

    if (ch == L'\r') {
      for (int i = Position; i < gInputSizeNow; i++) {
        wprintf(L"%lc", gInputBuffer[i]);
      }
      wprintf(L"\n");
      break;
    }

    // add char to gInputBuffer
    if (gInputSizeNow < BUF_SIZE) {
      for (int i = gInputSizeNow; i > Position; --i) {
        gInputBuffer[i] = gInputBuffer[i - 1];
      }
      gInputBuffer[Position] = ch;
      gInputSizeNow++;
      for (int i = Position; i < gInputSizeNow; i++) {
        wprintf(L"%lc", gInputBuffer[i]);
      }
      Position++;
      for (int i = Position; i < gInputSizeNow; i++) {
        is_chinese(gInputBuffer[i]) ? wprintf(L"\b\b") : wprintf(L"\b");
      }
    }
  }
  // gInputBuffer[gInputSizeNow] = L'\0';
  // for (int i = 0; i <= gInputSizeNow; i++) {
  //   wprintf(L"%02x ", gInputBuffer[i]);
  // }
  // wprintf(L"\n");
}

int main() {
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
  _setmode(_fileno(stdout), _O_U8TEXT);
  _setmode(_fileno(stdin), _O_U8TEXT);

  WSADATA Wsa;

  if (WSAStartup(MAKEWORD(2, 2), &Wsa) != 0) {
    wprintf(L"WSAStartup failed\n");
    return 1;
  }

  struct sockaddr_in ServerAddr;
  struct addrinfo Hints, *Res;
  wchar_t Name[100];
  char CharIpUnput[100];
  wchar_t IpInput[100];
  wchar_t PortInput[10];
  int Port;

  wprintf(L"Enter server IP: ");
  fgetws(IpInput, sizeof(IpInput) / sizeof(wchar_t), stdin);
  IpInput[wcscspn(IpInput, L"\n")] = 0;

  // -------------test case-------------
  if (IpInput[0] == 0) {
    memset(IpInput, 0, sizeof(IpInput));
    wcscpy(IpInput, IP);
  }
  // -----------------------------------

  WideCharToMultiByte(CP_UTF8, 0, IpInput, -1, CharIpUnput, sizeof(CharIpUnput), NULL, NULL);

  wprintf(L"Enter server Port: ");
  fgetws(PortInput, sizeof(PortInput) / sizeof(wchar_t), stdin);

  // -------------test case-------------
  if (PortInput[0] == L'\n') {
    Port = PORT;
  } else {
    wchar_t *endptr;
    Port = wcstol(PortInput, &endptr, 10);
  }
  // -----------------------------------

  do{
    wprintf(L"Enter your Name: ");
    fgetws(Name, sizeof(Name) / sizeof(wchar_t), stdin);
    Name[wcscspn(Name, L"\n")] = 0;
  } while (Name[0] == 0);

  memset (gChatTarget, 0, NAME_LEN);
  wcsncpy(gChatTarget, L"lobby", 6);

  gSock = socket(AF_INET, SOCK_STREAM, 0);
  if (gSock == INVALID_SOCKET) {
    wprintf(L"socket failed\n");
    return 1;
  }

  memset(&Hints, 0, sizeof(Hints));
  Hints.ai_family = AF_INET;
  Hints.ai_socktype = SOCK_STREAM;

  int err = getaddrinfo(CharIpUnput, NULL, &Hints, &Res);
  if (err != 0) {
    fwprintf(stderr, L"getaddrinfo: %s\n", gai_strerrorW(err));
    return 1;
  }

  ServerAddr = *(struct sockaddr_in *)Res->ai_addr;
  ServerAddr.sin_port = htons(Port);
  freeaddrinfo(Res);

  if (connect(gSock, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr)) < 0) {
    wperror_errno(L"connect ERROR");
    closesocket(gSock);
    WSACleanup();
    return 1;
  }

  // 送出使用者名稱，改用 UTF-8 送出
  send_utf8(gSock, Name);
  wprintf(L"Connected to server as %s.\n", Name);

  pthread_t recv_thread;
  pthread_create(&recv_thread, NULL, receive_handler, NULL);

  while (1) {
    wprintf(L"%s: ", gChatTarget);
    read_input_realtime();

    if (wcsncmp(gInputBuffer, L"/img ", 5) == 0) {
      gInputBuffer[wcscspn(gInputBuffer, L"\n")] = 0;
      send_image(gInputBuffer + 5);
      continue;
    } else if (wcscmp(gInputBuffer, L"/exit") == 0) {
      // send_utf8(gSock, gInputBuffer);
      break;
    }

    send_utf8(gSock, gInputBuffer);
  }

  closesocket(gSock);
  WSACleanup();
  return 0;
}
