#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

FILE* fin,fout;



void argumente(char** argv)
{
  struct stat st;
  if(stat(argv[1],&st)==-1)
    {
      perror("”Usage ./program <fisier_intrare>”.\n");
      exit(-1);
    }
  if(argv[2]!=NULL)
    {
      printf("”Usage ./program <fisier_intrare>”.\n");
      exit(-1);
    }
 
}

void print_name(char* nume_fisier)
{
  printf("1) Denumire fisier: %s\n",nume_fisier);
}

void print_dimension(int number)
{
  printf(" %d\n",number);
}

void width_and_height(char* nume_fisier)
{

}

int main(int argc, char** argv)
{
  argumente(argv);
  print_name(argv[1]);
  return 0;
}
