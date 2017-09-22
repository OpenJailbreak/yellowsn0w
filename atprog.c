/* Copyright iPhone DevTeam 2008 
 * Leet Hax not for commercial use.
 * DO NOT SELL - STRICTLY NO COMMERCIAL USE.
 * CUSTOM PACKETS MUST NOT BE USED IN ANY OTHER THIRD PARTY PRODUCTS.
 * YOU HAVE BEEN WARNED.
 * Punishment: Monkeys coming out of your ass Bruce Almighty style.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>

#define SPEED 115200

#pragma pack(1)

#define BUFSIZE (65536)
unsigned char readbuf[BUFSIZE];
struct termios term;

void SendCmd(int fd, void *buf, size_t size)
{
  if(write(fd, buf, size) == -1) {
    perror("write");
    exit(1);
  }
}

void SendStrCmd(int fd, char *buf)
{
  SendCmd(fd, buf, strlen(buf));
}

int ReadResp(int fd, unsigned int timeout_sec, unsigned int timeout_micro)
{
  int len = 0;
  struct timeval timeout;
  int nfds = fd + 1;
  fd_set readfds;

  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);

  timeout.tv_sec = timeout_sec;
  timeout.tv_usec = timeout_micro;

  struct timeval *to;
  if (timeout_sec==0 && timeout_micro==0) 
    to = NULL;
  else 
    to = &timeout;

  while (select(nfds, &readfds, NULL, NULL, to) > 0) {
    len += read(fd, readbuf + len, BUFSIZE - len);
  }
  return len;
}

int InitConn(int speed)
{
  unsigned int blahnull = 0;
  unsigned int handshake = TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;

  int fd = open("/dev/tty.debug", O_RDWR | 0x20000 | O_NOCTTY);
  if(fd == -1) {
    perror("open");
    exit(1);
  }

  ioctl(fd, 0x2000740D);
  fcntl(fd, 4, 0);
  tcgetattr(fd, &term);

  ioctl(fd, 0x8004540A, &blahnull);
  cfsetspeed(&term, speed);
  cfmakeraw(&term);
  term.c_cc[VMIN] = 0;
  term.c_cc[VTIME] = 5;

  term.c_iflag = (term.c_iflag & 0xFFFFF0CD) | 5;
  term.c_oflag =  term.c_oflag & 0xFFFFFFFE;
  term.c_cflag = (term.c_cflag & 0xFFFC6CFF) | 0x3CB00;
  term.c_lflag =  term.c_lflag & 0xFFFFFA77;

  term.c_cflag = (term.c_cflag & ~CSIZE) | CS8;
  term.c_cflag &= ~PARENB;
  term.c_lflag &= ~ECHO;

  tcsetattr(fd, TCSANOW, &term);

  ioctl(fd, TIOCSDTR);
  ioctl(fd, TIOCCDTR);
  ioctl(fd, TIOCMSET, &handshake);

  return fd;
}

void SendAT(int fd)
{
  SendStrCmd(fd, "AT\r");
}

void AT(int fd)
{
  SendAT(fd);
  
  while (1) {
    if(ReadResp(fd, 0, 500000) != 0) {
      break;
    }
    SendAT(fd);
  }
}

int atprog(char *cmds[], int num)
{
  int fd = InitConn(115200);
  AT(fd);
  int i;
  int retried;
  for (i=0; i<num; i++) {
    int len, txlen;
    char *buf;
    buf = malloc(strlen(cmds[i])+7);
    strcpy(buf, cmds[i]);
    if (buf[strlen(buf)-1]=='\n') {
      buf[strlen(buf)-1]='\0';
    }
    strcat(buf, "\r");
    txlen = strlen(buf);
    if (!txlen)
      continue;
    retried = -1;
  retry:
    retried++;
    if (retried>50)
      break;
    SendStrCmd(fd, buf);
    len = ReadResp(fd, 0, 700000);
    if (len<1) {
      AT(fd);  AT(fd);
      goto retry;
    }
    free(buf);
  }
  close(fd);

  return 0;
}

char *BasebandVersion(void)
{
  static char ret[16];
  ret[0] = ret[15] = '\0';
  int fd = InitConn(115200);
  AT(fd);
  int i;
  for (i=0; i<100; i++) {
    char *s;
    int len;
    SendStrCmd(fd, "AT+XGENDATA\r");
    len = ReadResp(fd, 0, 100000);
    if (!len) {
      continue;
    }
    if (len<1) {
      AT(fd);
      continue;
    }
    if ((s=strstr((char *)readbuf, "ICE2_MODEM_"))!=NULL) {
      strncpy(ret, s+11, 15);
      s = ret;
      while (*s) {
	if (*s=='"') {
	  *s = '\0';
	  break;
	}
	s++;
      }
      break;
    }
  }
  close(fd);
  return(ret);
}
