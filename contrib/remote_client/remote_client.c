#include <stdarg.h>
#include <pwd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "remote_client.h"

const char *pth_unix = ".gg2/.gg2_remote";
char *sock_path;
int sck = -1;

struct sockaddr_un sun;

int przybinduj (void)
{
  struct sockaddr_un _sun;

  _sun.sun_family = AF_UNIX;
  bzero (&_sun.sun_path, sizeof (_sun.sun_path));
  strcpy (_sun.sun_path + 1, "R");

  return bind (sck, (struct sockaddr *)&_sun, sizeof _sun);
}

int remote_init (void)
{
  struct passwd *pw;
  int yes = 1;

  pw = getpwuid (getuid ());
  if (pw == NULL)
    return -1;

  /* tu by sie przydalo sprawdzanie poprawnosci pw->pw_dir */
  sock_path = malloc (strlen (pw->pw_dir) + 1 + strlen (pth_unix) + 1);
  if (!sock_path)
    abort ();
  strcpy (sock_path, pw->pw_dir);
  strcpy (sock_path + strlen (pw->pw_dir) + 1, pth_unix);
  sock_path[strlen (pw->pw_dir)] = '/';

  sck = socket (PF_UNIX, SOCK_DGRAM, 0);
  if (sck == -1)
    goto abort;

  if (setsockopt (sck, SOL_SOCKET, SO_PASSCRED, &yes, sizeof (int)) == -1)
    goto abort;
  
  sun.sun_family = AF_UNIX;
  bzero (&sun.sun_path, sizeof (sun.sun_path));
  strcpy (sun.sun_path, sock_path);

  if (przybinduj () == -1)
    goto abort;

  if (connect (sck, (struct sockaddr *)&sun, sizeof sun) == -1)
    goto abort;

  return 0;

abort:
    printf ("%s\n", strerror (errno));
    if (sck != -1)
    {
      close (sck);
      sck = -1;
    }
  return -1;

}

int remote_send (char *text)
{
  return remote_send_data (text, strlen (text) + 1);
}

int remote_vsend (int count, ...)
{
  va_list ap;
  char *text = NULL;
  int textlen = 0;
  char *s;
  int i;

  va_start (ap, count);
  while (count--)
  {
    s = va_arg (ap, char *);
    if (!text)
    {
      textlen = strlen (s) + 1;
      text = malloc (textlen);
      if (!text)
	abort ();
      strcpy (text, s);
      s[textlen - 1] = '\0';
    } else
    {
      i = strlen (s) + 1;
      text = realloc (text, textlen + i);
      if (!text)
	abort ();

      strcpy (text + textlen - 1, s);
      textlen += i;
      text [textlen - 1] = '\0';
    }
  }

  va_end (ap);

  i = remote_send_data (text, textlen);
  free (text);
  return i;
}

int remote_send_data (char *data, int len)
{
  struct msghdr msg;
  struct cmsghdr *cmsg;
  char buf[CMSG_SPACE (sizeof (struct ucred))];
  struct ucred *pcred;
  struct iovec iov;

  printf ("%s\n", data);
  
  msg.msg_control = buf;
  msg.msg_controllen = sizeof (buf);

  cmsg = CMSG_FIRSTHDR (&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type  = SCM_CREDENTIALS;
  cmsg->cmsg_len   = CMSG_LEN (sizeof (struct ucred));

  pcred = (struct ucred *) CMSG_DATA (cmsg);
  pcred->pid = getpid ();
  pcred->uid = getuid ();
  pcred->gid = getgid ();

  msg.msg_name    = &sun;
  msg.msg_namelen = sizeof (sun);

  iov.iov_base = data;
  iov.iov_len  = len;
  
  msg.msg_iov    = &iov;
  msg.msg_iovlen = 1;

  return sendmsg (sck, &msg, 0);
}

int remote_close (void)
{
  shutdown (sck, SHUT_RDWR);
  close (sck);
  sck = -1;
  return 0;
}

int remote_fd (void)
{
  return sck;
}

int remote_recv (char *buf, int maxlen)
{
  int len;

  struct sockaddr_un *csun;
  int csunlen;
  struct msghdr msg;
  struct cmsghdr *cmsg = NULL;
  struct ucred *pcred;
  struct iovec iov[1];
  char mbuf[CMSG_SPACE(sizeof (struct ucred))];

  memset (&msg, 0, sizeof msg);
  memset (mbuf, 0, sizeof mbuf);

  msg.msg_name    = NULL;
  msg.msg_namelen = 0;

  iov[0].iov_base = buf;
  iov[0].iov_len  = maxlen;

  msg.msg_iov    = iov;
  msg.msg_iovlen = 1;

  msg.msg_control    = mbuf;
  msg.msg_controllen = sizeof mbuf;

  if ((len = recvmsg (sck, &msg, 0)) == -1)
  {
    fprintf (stderr, "remote_client:remote_recv():%s\n", strerror (errno));
    return -1;
  }

  /* szukamy w danych dodatkowych przes³anych uwierzytelnieñ */
  for (cmsg = CMSG_FIRSTHDR (&msg); cmsg != NULL;
      cmsg = CMSG_NXTHDR (&msg, cmsg))
  {
    if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_CREDENTIALS)
    {
      pcred = (struct ucred *) CMSG_DATA (cmsg);
      break;
    }
  }
   
  if (cmsg == NULL)
  {
    /* nie znale¼li¶my ¿adnych uwierzytelnieñ, wiêc ignorujemy pakiet */
    fprintf(stderr,"remote_client:remote_recv(): no credentials, discarding\n");
    return -1;
  }

  /* sprawdzenie poprawno¶ci danych */
  if (len == 0) /* nie mo¿e byæ */
  {
    fprintf (stderr, "remote_client:remote_recv(): recvmsg() = 0, a nie powinno\n");
    return -1;
  }
/*
  if (buf[len - 1] != '\0')
  {
    fprintf (stderr, "remote_client:remote_recv(): b³êdny pakiet\n");
    return -1;
  }
*/
  csun = msg.msg_name;
  csunlen = msg.msg_namelen;
  
  return len;
}

int remote_sig_quit                 (void)
{
  return remote_send ("quit");
}

int remote_sig_gui_show             (void)
{
  return remote_send ("gui show");
}

int remote_sig_gui_warn             (char *text)
{
  return remote_vsend (2, "gui warn ", text);
}

int remote_sig_gui_msg              (char *text)
{
  return remote_vsend (2, "gui msg ", text);
}

int remote_sig_gui_notify           (char *text)
{
  return remote_vsend (2, "gui msg ", text);
}

int remote_sig_xosd_show            (char *text)
{
  return remote_vsend (2, "xosd show ", text);
}

int remote_sig_remote_get_icon      (char **filepath)
{
  char buf[512];
  int i;
  if (remote_send ("remote get_icon") == -1)
    return -1;

  if ((i = remote_recv ((char *)buf, 512)) == -1)
    return -1;
  printf ("%d, %s\n", i, buf);

  if (buf[0] != 'o' && buf[1] != 'k' && buf[2] != ' ')
    return -1;

  *filepath = malloc (i - 3);
  if (!*filepath)
    abort ();

  strncpy (*filepath, buf + 3, i);

  return 0;
}

int remote_sig_remote_get_icon_path (char **filepath)
{
  char buf[512];
  int i;
  if (remote_send ("remote get_icon_path") == -1)
    return -1;

  if ((i = remote_recv ((char *)buf, 512)) == -1)
    return -1;

  if (buf[0] != 'o' && buf[1] != 'k' && buf[2] != ' ') /* error */
    return -1;

  *filepath = malloc (strlen (buf) - 3);
  if (!*filepath)
    abort ();

  strcpy (*filepath, buf + 3);

  return 0;
}

