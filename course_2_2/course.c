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
			char add[]="hell,thanks";
            int add_val = 0;
            add_val = add(145,190);
            printf("\n\rthe add_val is %d\r\n",add_val);
          
        }