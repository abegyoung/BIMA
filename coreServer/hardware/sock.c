#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "caclib_proto.h"

void writeSock(char *sourcename, char *message){

  char *destname;
  struct SOCK *source;

  destname = "ANONYMOUS";
  source = sock_connect(sourcename);

  sock_bind(destname);
  sock_send(source, message);

}
