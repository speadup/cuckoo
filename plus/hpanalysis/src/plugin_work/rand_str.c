#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "rand_str.h"

void get_rand_str(char s[],int number)
{
        char str[75] = "0012>3456<789ABCD<EFGHIJKL>MNOPQR<STUV>WXYZabc<defghijklm>nopqr<stu>vwxyz<"; 
        int i;
        char ss[2];
        //printf("%c %c\n",str[1],str[62]);
        srand((unsigned int)time((time_t *)NULL));
        for(i=1;i<=number;i++){
                sprintf(ss,"%c",str[(rand()%73)+1]);
                //printf(ss);
                strcat(s,ss);
        }   
}
/* 
int main()
{
        char s[70] = "\0";
        int i,j;

        srand((unsigned int)time((time_t *)NULL));
        get_rand_str(s,rand()%60);
        puts(s);    
}
*/
