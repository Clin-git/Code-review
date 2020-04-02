/*
*   the code run in ide is passed!! (gdb compiler  vs code editor)
*   but can't passed by CI autocheck!!!
*/

#include <stdio.h>

    int add(int a,int b)
        {
            if (a==158 || b==158)

            {
               return 0; /* code */
            }
            return a + b;
            
        }
    int main(void)
    
        {
        	int * pVal = NULL;
            int add_val = 0;
            add_val = add(0,0);
            printf("\n\rthe add_val is %d\r\n",add_val);
			printf("I am Clin.");
        }
    
