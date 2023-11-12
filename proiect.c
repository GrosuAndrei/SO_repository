#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>


struct stat status(char** argv)
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
  return st;
}

int get_img_descriptor(char* nume_fisier)
{
  int acces_img=open(nume_fisier,O_RDONLY);
    if(acces_img==-1)
    {
      perror("Imaginea nu poate fi accesata\n");
      exit(-1);
    }
    return acces_img;
}
  
char* width_and_height(int img_descriptor)
{
  char* buffer=malloc(100*sizeof(char));
  read(img_descriptor,buffer,18);
  read(img_descriptor,buffer,4);
  char* Width=malloc(4*sizeof(char));
  strcpy(Width,buffer);
  read(img_descriptor,buffer,4);
  char* Height=malloc(4*sizeof(char));
  strcpy(Height,buffer);
  strcat(Width," ");
  strcat(Width,Height);
  return Width;// "width height" concatenate (teoretic)
}

int get_wr_descriptor()
{
  int wr_descriptor=open("statistica.txt",O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if(wr_descriptor==-1)
    {
      perror("Fisierul destinatie nu a putut fi deschis\n");
      exit(-1);
    }
  return wr_descriptor;
}

int get_total_size(struct stat img_st)
{
  return img_st.st_size;
}

int get_user_id(struct stat img_st)
{
  return img_st.st_uid;
}

char* get_last_mod_time(struct stat img_st)
{
  struct timespec ts=img_st.st_atim;
  timespec_get(&ts, TIME_UTC);
  char buff[100];
  strftime(buff, sizeof buff, "%D %T", gmtime(&ts.tv_sec));
  char* rezultat=malloc(100*sizeof(char));
  strcpy(rezultat,buff);
  return rezultat;
}

int get_number_of_hard_links(struct stat img_st)
{
    return img_st.st_nlink;
}

void strmode(mode_t mode, char * buf) {
  const char chars[] = "RWXRWXRWX";
  for (size_t i = 0; i < 9; i++) {
    buf[i] = (mode & (1 << (8-i))) ? chars[i] : '-';
  }
  buf[9] = '\0';
}

void substrings(int min, int max, char* str, char* substr)
{
  int n=max-min;
  int i;
  for(i=0;i<=n;i++)
    {
      substr[i]=str[min+i];
    }
}

void transfer_data(int wr_descriptor,struct stat img_st, int img_descriptor, char* nume_fisier)
{
  char* buffer=malloc(100*sizeof(char));
  char dimensiuni[100];
  strcpy(dimensiuni,width_and_height(img_descriptor));
  char* lungime=strtok(dimensiuni," ");
  char* inaltime=strtok(NULL,"\n");
  sprintf(buffer,"nume fisier: %s\ninaltime: %s\nlunigime: %s",nume_fisier,inaltime,lungime);
  printf("%s\n",buffer);
  if(write(wr_descriptor,buffer,sizeof(buffer))==-1)
  {
    perror("esec la scriere 1");
    exit(-1);
  }
  sprintf(buffer,"dimensiune: %d\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s",get_total_size(img_st),get_user_id(img_st),get_last_mod_time(img_st));
  printf("%s\n",buffer);
  if(write(wr_descriptor,buffer,sizeof(buffer))==-1)
  {
    perror("esec la scriere 2");
    exit(-1);
  }
  char buf[10];
  strmode(img_st.st_mode,buf);
  char User_mode[5];
  substrings(0,2,buf,User_mode);
  char Group_mode[5];
  substrings(3,5,buf,Group_mode);
  char Other_mode[5];
  substrings(6,8,buf,Other_mode);
  sprintf(buffer,"contorul de legaturi: %d\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s",get_number_of_hard_links(img_st),User_mode,Group_mode,Other_mode);
  if(write(wr_descriptor,buffer,sizeof(buffer))==-1)
  {
    perror("esec la scriere 3");
    exit(-1);
  }
}

int main(int argc, char** argv)
{
  struct stat img_st=status(argv);
  int img_descriptor=get_img_descriptor(argv[1]);
  int wr_descriptor=get_wr_descriptor();
  //transfer_data(wr_descriptor,img_st,img_descriptor,argv[1]);
  return 0;
}
