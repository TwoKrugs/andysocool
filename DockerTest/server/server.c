// multi_server.c (with anti-spam)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <stdbool.h>

#define PORT         12345
#define MAX_CLIENTS  10
#define NAME_LEN     100
#define BUF_SIZE     2048
#define LOBBY_NAME   "lobby"

typedef struct {
  int                   socket;
  struct sockaddr_in    addr;
  bool                  ban;
  time_t                ban_until;
  time_t                msg_times[6];
  int                   msg_index;
  char                  name[NAME_LEN];
  int                   private_chat;
  char                  private_chat_name[NAME_LEN];
  bool                  is_server_message;
} client_info;

client_info      clients[MAX_CLIENTS];
pthread_mutex_t  lock = PTHREAD_MUTEX_INITIALIZER;

char *
convert_image_to_ascii (
  const char  *image_data,
  int         image_size
  )
{
  int  in_pipe[2];
  int  out_pipe[2];

  if ((pipe (in_pipe) == -1) || (pipe (out_pipe) == -1)) {
    perror ("pipe failed");
    return NULL;
  }

  pid_t  pid = fork ();
  if (pid == -1) {
    perror ("fork failed");
    return NULL;
  }

  if (pid == 0) {
    dup2 (in_pipe[0], STDIN_FILENO);
    dup2 (out_pipe[1], STDOUT_FILENO);
    close (in_pipe[1]);
    close (out_pipe[0]);
    execlp ("jp2a", "jp2a", "--width=80", "-", NULL);
    perror ("exec failed");
    exit (1);
  } else {
    close (in_pipe[0]);
    close (out_pipe[1]);

    write (in_pipe[1], image_data, (unsigned int)image_size);
    close (in_pipe[1]);

    char  *ascii = malloc (image_size);
    ascii[0] = '\0';
    char     buffer[256];
    ssize_t  n;
    while ((n = read (out_pipe[0], buffer, sizeof (buffer) - 1)) > 0) {
      buffer[n] = '\0';
      strcat (ascii, buffer);
    }

    close (out_pipe[0]);
    wait (NULL);
    return ascii;
  }
}


bool
check_chat (
  int  chat
)
{
  if (chat == 0){
    return true;
  }
  bool is_found = false;
  pthread_mutex_lock (&lock);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if ((clients[i].socket != 0) && (chat == clients[i].socket)) {
      is_found = true;
      break;
    }
  }
  pthread_mutex_unlock (&lock);
  return is_found;
}

void
send_message (
  char  *msg,
  int   receive_fd
  )
{
  // 計算原始訊息長度
  size_t  msg_len = strlen (msg);

  // 建立前綴長度字串，例如 LEN=0012|
  char  prefix[20];                                          // 足夠容納前綴

  snprintf (prefix, sizeof (prefix), "LEN=%04zu|", msg_len); // 固定4位長度

  // 建立完整訊息字串
  size_t  full_len  = strlen (prefix) + msg_len + 1;
  char    *full_msg = malloc (full_len);
  if (!full_msg) {
    perror ("malloc failed");
    return;
  }

  strcpy (full_msg, prefix);
  strcat (full_msg, msg);

  send (receive_fd, full_msg, strlen (full_msg), 0);

  free (full_msg);
}

void
private_message (
  char  *msg,
  int   receive_fd
)
{
  size_t length  = strlen(msg) + 4;
  char  *new_msg = malloc(length);
  snprintf (new_msg, length, "/p%s", msg);
  send_message (new_msg, receive_fd);
  free (new_msg);
}

void
broadcast_message (
  char  *msg,
  int   sender_fd
  )
{
  pthread_mutex_lock (&lock);

  // 廣播訊息
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if ((clients[i].socket != 0) && (clients[i].socket != sender_fd)) {
      send_message (msg, clients[i].socket);
    }
  }

  pthread_mutex_unlock (&lock);
}

void
message_hob (
  char  *msg,
  client_info  *client
)
{
  if (client->is_server_message) {
    send_message (msg, client->socket);
    client->is_server_message = false;
  } else if (client->private_chat == 0) {
    printf ("%s", msg);
    broadcast_message (msg, client->socket);
  } else {
    if (check_chat(client->private_chat) == false){
      char fail_msg[150] = "";
      snprintf (fail_msg, 150, "/private%s|User [%s] Not Found! You Are Lobby Now.\n", LOBBY_NAME, client->private_chat_name);
      send_message (fail_msg, client->socket);
      client->private_chat = 0;
      strcpy (client->private_chat_name, LOBBY_NAME);
    } else {
      printf ("%s", msg);
      private_message (msg, client->private_chat);
    }
  }
}

char *
handle_messages (
  client_info  *client,
  int          client_fd,
  char         *buffer
  )
{
  char  *msg = NULL;

  if (strncmp (buffer, "IMAGE:", 6) == 0) {
    int   size      = atoi (buffer + 6);
    char  *img_data = malloc (size);
    int   received  = 0;
    while (received < size) {
      int  r = recv (client_fd, img_data + received, size - received, 0);
      if (r <= 0) {
        break;
      }

      received += r;
    }

    char  *ascii = convert_image_to_ascii (img_data, size);

    size_t  new_msg_size = strlen (ascii) + strlen (client->name) + 16;
    msg = malloc (new_msg_size);
    snprintf (msg, new_msg_size, "[%s]\n%s", client->name, ascii);

    free (ascii);
    free (img_data);
  } else if (strncmp (buffer, "/memberlist", 11) == 0) {
    client->is_server_message = true;
    size_t  new_msg_size = NAME_LEN * MAX_CLIENTS + 64; // 多留一點空間
    msg = malloc (new_msg_size);

    memset (msg, 0, new_msg_size);

    size_t  used = snprintf (msg, new_msg_size, "Member List: ");

    pthread_mutex_lock (&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clients[i].socket != 0) {
        // 追加使用 snprintf，保證不溢位
        printf ("%s\n", clients[i].name);
        used += snprintf (msg + used, new_msg_size - used, "[%s] ", clients[i].name);
        printf ("%s\n", msg);
      }
    }

    pthread_mutex_unlock (&lock);
    printf ("%s\n", msg);
    snprintf (msg + used, new_msg_size - used, "\n");  // 安全補上換行
    printf ("%s", msg);
  } else if (strncmp (buffer, "/chat ", 6) == 0) {
    client->is_server_message = true;
    size_t  new_msg_size = BUF_SIZE;
    msg = malloc (new_msg_size);
    memset (msg, 0, new_msg_size);
    char  *new_chat = strstr (buffer, " ");
    if (new_chat) {
      new_chat++;
    }

    client->private_chat = 0;
    strcpy (client->private_chat_name, LOBBY_NAME);
    pthread_mutex_lock (&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if ((clients[i].socket != 0) && (strcmp (clients[i].name, new_chat) == 0)) {
        client->private_chat = clients[i].socket;
        strcpy(client->private_chat_name, clients[i].name);
        break;
      }
    }

    pthread_mutex_unlock (&lock);
    if (client->private_chat != 0) {
      snprintf (msg, new_msg_size, "/private%s|You are chat with %s\n", new_chat, new_chat);
    } else {
      if (strcmp (new_chat, "lobby") == 0){
        snprintf (msg, new_msg_size, "/private%s|You are lobby.\n", LOBBY_NAME);
      } else {
        snprintf (msg, new_msg_size, "/private%s|Not found user \"%s\", you are lobby.\n", LOBBY_NAME, new_chat);
      }
    }
  } else {
    time_t  now = time (NULL);
    if (client->ban && (now < client->ban_until)) {
      client->is_server_message = true;
      int   remaining = (int)(client->ban_until - now);
      int   msg_size = 128;
      msg = malloc (msg_size);
      snprintf (msg, msg_size, "你還剩下 %d 秒才能說話,你好爛.\n", remaining);
      return msg;
    }

    client->msg_times[client->msg_index] = now;
    client->msg_index                    = (client->msg_index + 1) % 5;
    int  recent_count = 0;
    for (int i = 0; i < 5; i++) {
      if (now - client->msg_times[i] <= 5) {
        recent_count++;
      }
    }

    if (recent_count >= 5) {
      client->is_server_message = true;
      int   msg_size = 128;
      msg = malloc (msg_size);
      client->ban       = true;
      client->ban_until = now + 15;
      printf ("[%s] 這位用戶很皮被ban了.\n", client->name);
      snprintf (msg, msg_size, "還敢洗頻阿,拉基,15秒好好反省.\n");
      return msg;
    }

    size_t  new_msg_size = strlen (buffer) + strlen (client->name) + 16;
    msg = malloc (new_msg_size);
    snprintf (msg, new_msg_size, "[%s] %s\n", client->name, buffer);
  }

  return msg;
}

void *
handle_client (
  void  *arg
  )
{
  client_info  *client   = (client_info *)arg;
  int          client_fd = client->socket;
  char         buffer[BUF_SIZE];
  char         *msg = NULL;

  size_t  msg_size = BUF_SIZE;

  msg = malloc (msg_size);

  snprintf (msg, msg_size, "[%s] joined the chat.\n", client->name);
  printf ("%s", msg);
  broadcast_message (msg, client_fd);

  free (msg);

  while (1) {
    memset (buffer, 0, sizeof (buffer));
    int  bytes = recv (client_fd, buffer, sizeof (buffer), 0);
    if (bytes <= 0) {
      msg = malloc (msg_size);
      snprintf (msg, msg_size, "[%s] left the chat.\n", client->name);
      printf ("%s", msg);
      broadcast_message (msg, client_fd);
      close (client_fd);
      pthread_mutex_lock (&lock);
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_fd) {
          clients[i].socket = 0;
          break;
        }
      }

      pthread_mutex_unlock (&lock);
      free (msg);
      free (client);
      break;
    } else {
      msg = handle_messages (client, client_fd, buffer);
    }

    if (msg != NULL){
      message_hob(msg, client);
      free(msg);
    }
  }

  return NULL;
}

int
main (
  )
{
  int                 server_fd;
  struct sockaddr_in  server_addr, client_addr;
  socklen_t           addr_len = sizeof (client_addr);

  memset (clients, 0, sizeof (clients));
  server_fd = socket (AF_INET, SOCK_STREAM, 0);

  server_addr.sin_family      = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port        = htons (PORT);

  bind (server_fd, (struct sockaddr *)&server_addr, sizeof (server_addr));
  listen (server_fd, MAX_CLIENTS);
  printf ("Server is listening on port %d...\n", PORT);

  while (1) {
    int  client_fd = accept (server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0) {
      perror ("accept failed");
      continue;
    }

    client_info  *new_client = malloc (sizeof (client_info));
    memset (new_client, 0, sizeof(client_info));

    new_client->socket = client_fd;
    new_client->addr = client_addr;
    memset(new_client->name, 0, NAME_LEN);
    new_client->private_chat = 0;
    strcpy (new_client->private_chat_name, LOBBY_NAME);
    new_client->is_server_message = false;

    memset (new_client->name, 0, NAME_LEN);
    recv (client_fd, new_client->name, NAME_LEN, 0);

    pthread_mutex_lock (&lock);
    int  stored = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clients[i].socket == 0) {
        clients[i] = *new_client;
        stored     = 1;
        break;
      }
    }

    pthread_mutex_unlock (&lock);

    if (!stored) {
      printf ("Max clients reached. Connection rejected.\n");
      close (client_fd);
      continue;
    }

    pthread_t  thread_id;
    pthread_create (&thread_id, NULL, handle_client, new_client);
    pthread_detach (thread_id);
  }

  close (server_fd);
  return 0;
}
