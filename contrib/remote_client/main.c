#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "remote_client.h"

int main (int argc, char *argv[])
{
  char *s;
    
  if (remote_init () == -1)
    return -1;
    
  if (argc > 1) { remote_send(argv[1]); return 0; }
  
  remote_sig_remote_get_icon (&s);
  printf ("get_icon -> %s\n", s);
  free (s);
  remote_sig_remote_get_icon_path (&s);
  printf ("get_icon_path -> %s\n", s);
  free (s);
  
  remote_close ();
  
  return 0;
}
