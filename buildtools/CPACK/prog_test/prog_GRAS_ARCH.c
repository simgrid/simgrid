//This programme check size of datatypes 

#include <sys/types.h>
#include <stdio.h>

int main (void)
{

	int c = sizeof(char);
	int si = sizeof(short int);
	int i = sizeof(int);
	int li = sizeof(long int);
	int lli = sizeof(long long int);
	int f = sizeof(float);
	int v = sizeof(void *);
	int vv = sizeof(void (*) (void));
	/*printf("char : %d\n",c);
	printf("short int : %d\n",si);
	printf("int : %d\n",i);
	printf("long int : %d\n",li);
	printf("long long int : %d\n",lli);
	printf("float : %d\n",f);
	printf("void * : %d\n",v);
	printf("void (*) (void) : %d\n",vv);*/

	struct s0 {char c0; char i0;};
	struct s1 {char c1; short int i1;};
	struct s2 {char c2; int i2;};
	struct s3 {char c3; long int i3;};
	struct s4 {char c4; long long int i4;};
	struct s5 {char c5; double i5;};
	struct s6 {char c6; void * i6;};
	int res0=sizeof(struct s0)-sizeof(char);
	int res1=sizeof(struct s1)-sizeof(short int);
	int res2=sizeof(struct s2)-sizeof(int);
	int res3=sizeof(struct s3)-sizeof(long int);
	int res4=sizeof(struct s4)-sizeof(long long int);
	int res5=sizeof(struct s5)-sizeof(double);
	int res6=sizeof(struct s6)-sizeof(void *);
	/*printf("struct-char : %d\n",res0);
	printf("struct-short int : %d\n",res1);	
	printf("struct-int : %d\n",res2);	
	printf("struct-long int : %d\n",res3);	
	printf("struct-long long int : %d\n",res4);	
	printf("struct-double : %d\n",res5);	
	printf("struct-void * : %d\n",res6);*/

	printf("_C:%d/%d:_I:%d/%d:%d/%d:%d/%d:%d/%d:_P:%d/%d:%d/%d:_D:4/%d:8/%d:",c,res0,si,res1,i,res2,li,res3,lli,res4,v,res6,vv,res6,f,res5);
	return 1;
}

