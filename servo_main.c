#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <wiringPi.h>

/*Gloval Variables*/

int n; //the number of bytes 
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

void Servo(int pin, char *buf){
    int cont;
    write(1, buf, n);
    if(strcmp(buf,"low1") == 0){
	cont = 184;
	softPwmWrite(pin,cont);
	printf("the value is %d\n",cont);
    } else if(strcmp(buf,"low2") == 0){
	cont = 186;
	softPwmWrite(pin,cont);
	printf("the value is %d\n",cont);
    } else if(strcmp(buf,"mid1") == 0){
	cont = 188;
	softPwmWrite(pin,cont);
	printf("the value is %d\n",cont);
    } else if(strcmp(buf,"mid2") == 0){
	cont = 190;
	PwmWrite(pin,cont);
	printf("the value is %d\n",cont);
    } else if(strcmp(buf,"hig1") == 0){
	cont = 192;
	softPwmWrite(pin,cont);
	printf("the value is %d\n",cont);
    } else if(strcmp(buf,"hig2") == 0){
	cont = 194;
	softPwmWrite(pin,cont);
	printf("the value is %d\n",cont);
    } else {
	softPwmWrite(pin,180);
    }
}

int main (int argc, char *argv[])
{
    char buf[4];
    if(argc == 2) InitServer(atoi(argv[1]));
    int pin = 0; //GPIO 17
    if (softPwmCreate(pin,0,200) != 0){
	fprintf (stdout, "oops: %s\n", strerror (errno)) ;
	return 1 ;
    }
    
    while(1){
	while((n = read(sock,buf,4)) > 0){
	    Servo(pin, buf);
        }
    }

    return 1;
}
