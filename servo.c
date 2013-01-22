#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <wiringPi.h>

/*Gloval Variable*/

int n; //filediscpritor
int sock; //socket discpritor


void InitServer(int port){
    struct hostent *host;
    struct sockaddr_in me;
    int s0;
    char buf[512];
    s0 = socket(PF_INET, SOCK_STREAM, 0);
    bzero((char *)&me, sizeof(me));
    me.sin_family = PF_INET;
    me.sin_port = htons(port);
    if(bind(s0,(struct sockaddr *)&me, sizeof(me)) < 0){
	fprintf(stderr,"cannot bind socket\n");
	exit(1);
    }
    listen(s0,1);
    sock = accept(s0,NULL, NULL);
}

void Servo(int pin, char buf){
    write(1, buf, n);
    int val = atoi(buf);
printf("value = %d\n",val);
    digitalWrite(pin,val);
}

int main (int argc, char *argv[])
{
    char buf[512];
    if(argc == 2) InitServer(atoi(argv[1]));
    
    if (wiringPiSetup () == -1){
	fprintf (stdout, "oops: %s\n", strerror (errno)) ;
	return 1 ;
    }
    int pin = 0; //GPIO 17
    pinMode(pin, OUTPUT);

    while(1){
	while((n = read(sock,buf,512)) > 0){
	    Servo(pin, buf[512]);
        }
    }

    return 1;
}
