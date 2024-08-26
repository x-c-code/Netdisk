#include "head.h"

int initTcpSocket(int *socketfd,char *ip,char *port){

  *socketfd = socket(AF_INET,SOCK_STREAM,0);

  int reuse = 1;
  setsockopt(*socketfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));

  //构建sockaddr_in addr
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip);
  addr.sin_port = htons(atoi(port));
  connect(*socketfd,(struct sockaddr *)&addr,sizeof(addr));
  return 0;
}
