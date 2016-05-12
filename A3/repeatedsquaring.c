
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int repeatsquare(unsigned long int x, unsigned long int e, unsigned long int n){
  unsigned long int y=1;
  while(e>0){
	if((e%2)==0){
		printf("x=%ld \n",x);
		x=(x*x)%n;
		e=e/2;
		
	}
	else{
		printf("x=%ld y=%ld \n",x,y);
		y=(x*y)%n;
		e=e-1;
	}
  }
  return y;
}


main(int argc, char *argv[]){
int x,e,n;
if (argc!=4) {printf("need x,e,n \n");exit(0);}
x=atoi(argv[1]);
e=atoi(argv[2]);
n=atoi(argv[3]);
printf("%d^%d mod %d   is  %d \n",x,e,n,repeatsquare(x,e,n));
printf(" size int %ld size long int %ld \n",sizeof(int),sizeof(long int));
}
