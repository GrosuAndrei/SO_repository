#include <stdio.h

#include <stdio.h>

#include <string.h>

#include <errno.h>

#include <sys/stat.h>

#include <sys/types.h>

#include <fcntl.h>

#include <unistd.h>

#include <time.h>

#include <dirent.h>

#include <libgen.h>



// 1*****************************************************************************************



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

  int* Width=malloc(sizeof(int));

  read(img_descriptor,Width,4);

  buffer[4]='\0';

  int* Height=malloc(sizeof(int));

  read(img_descriptor,Height,4);

  printf("%d\n",*Width);

  char* result=malloc(100*sizeof(char));

  sprintf(result,"%d %d",*Width,*Height);

  return result;// "width height" concatenate (teoretic)

}



int get_wr_descriptor() {

  int wr_descriptor = open("statistica.txt", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (wr_descriptor == -1) {

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



void substrings(int offset, int len, char* str, char* substr)

{

  int i,j;

  for(i=0,j=offset;i<len;i++,j++)

    {

      substr[i]=str[j];

    }

  substr[i]='\0';

}



void write_access_rights(int wr_descriptor, struct stat entry_st)

{

  char result[1000];

  char user_mode[4];

  char group_mode[4];

  char other_mode[4];

  char buf[9];

  strmode(entry_st.st_mode,buf);

  substrings(0,3,buf,user_mode);

  substrings(3,3,buf,group_mode);

  substrings(6,3,buf,other_mode);

  sprintf(result,"drepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii:%s\n",user_mode,group_mode,other_mode);

  printf("%s\n",result);

  if(write(wr_descriptor, result, strlen(result))==-1)

    {

      perror(" access rights error \n");

      exit(-1);

    }

}



// 1*********************************************************************************************



// 2*********************************************************************************************

void transfer_data_bmp(int wr_descriptor, struct stat entry_st, int img_descriptor, char* entry_name)

{

  char buffer[1000];

  char dimensiuni[100];

  strcpy(dimensiuni,width_and_height(img_descriptor));

  char* lungime=strtok(dimensiuni," ");

  char* inaltime=strtok(NULL,"\n");

  sprintf(buffer,"nume fisier: %s\ninaltime: %s\nlunigime: %s\n",entry_name,inaltime,lungime);

  if(write(wr_descriptor,buffer,strlen(buffer))==-1)

    {

      perror(" first write bmp file\n");

      exit(-1);

    }

  sprintf(buffer,"dimensiune: %d\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\ncontorul de legaturi: %d\n",get_total_size(entry_st),get_user_id(entry_st),get_last_mod_time(entry_st), get_number_of_hard_links(entry_st));

    if(write(wr_descriptor,buffer,strlen(buffer))==-1)

    {

      perror(" second write bmp file\n");

      exit(-1);

    }

    write_access_rights(wr_descriptor,entry_st);

}



void transfer_data_not_bmp(int wr_descriptor,struct stat entry_st, char* entry_name)

{

  char buffer[1000];

  sprintf(buffer, "\nnume fisier: %s\ndimensiune: %d\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\n",entry_name, get_total_size(entry_st), get_user_id(entry_st), get_last_mod_time(entry_st));

  if (write(wr_descriptor, buffer, strlen(buffer)) == -1)

    {

      perror("first write not bmp file\n");

      exit(-1);

    }

  write_access_rights(wr_descriptor, entry_st);

}



void transfer_data_symblink(int wr_descriptor, struct stat entry_st, struct stat target_file_st, char* entry_name)

{

  char buffer[1000];

  sprintf(buffer, "\nnume legatura: %s\ndimensiune legatura: %d\ndimensiune fisier: %d\n", entry_name, get_total_size(entry_st), get_total_size(target_file_st));

  if (write(wr_descriptor, buffer, strlen(buffer)) == -1) {

    perror("first write to symblink file\n");

    exit(-1);

  }

  write_access_rights(wr_descriptor,entry_st);

}



void transfer_data_directory(int wr_descriptor, struct stat entry_st, char* entry_name)

{

  char buffer[1000];

   sprintf(buffer, "\nnume director: %s\nidentificatorul utilizatorului: %d\n", entry_name, get_user_id(entry_st));

  if (write(wr_descriptor, buffer, strlen(buffer)) == -1) {

    perror("Esec la scriere director");

    exit(-1);

  }

  write_access_rights(wr_descriptor,entry_st);

}



// 2********************************************************************************************



// 3********************************************************************************************

void process_entry(int wr_descriptor, char* entry_name, char* dir_path) {

  char entry_path[255];

  snprintf(entry_path,sizeof(entry_path), "%s/%s", dir_path, entry_name);

  struct stat entry_st;

  if (lstat(entry_path, &entry_st) == -1) {

    perror("Eroare la stat process_entry");

    exit(-1);

  }

  if (S_ISREG(entry_st.st_mode)) {

    if (strcmp(entry_name + strlen(entry_name) - 4, ".bmp") == 0) {

    // Fișier cu extensia .bmp

      int img_descriptor=get_img_descriptor(entry_path);

      transfer_data_bmp(wr_descriptor,entry_st,img_descriptor,entry_name);

    } else {

      // Altfel, fișier obișnuit fără extensie .bmp

      transfer_data_not_bmp(wr_descriptor,entry_st,entry_name);

    }

  } else if (S_ISLNK(entry_st.st_mode)) {

    // Legătură simbolică

     struct stat target_file_st;

  if (stat(entry_path, &target_file_st) == -1)

    {

      perror("Eroare la stat process_entry");

      exit(-1);

    }

    transfer_data_symblink(wr_descriptor,entry_st,target_file_st,entry_name);

  } else if (S_ISDIR(entry_st.st_mode)) {

    // Director

     transfer_data_directory(wr_descriptor,entry_st, entry_name);

  }

}





void process_directory(int wr_descriptor, char* dir_path) {

  DIR* dir;

  struct dirent* entry;

  dir = opendir(dir_path);

  if (dir == NULL) {

    perror("Eroare la deschiderea directorului");

    exit(-1);

  }

  while ((entry = readdir(dir)) != NULL) {

    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {

      process_entry(wr_descriptor, entry->d_name, dir_path);

    }

  }

  closedir(dir);

}

// 3****************************************************************************************



int main(int argc, char** argv) {

  if (argc != 2) {

    printf("Usage: %s <director_intrare>\n", argv[0]);

    exit(-1);

  }



  struct stat dir_st;

  if (stat(argv[1], &dir_st) == -1 || !S_ISDIR(dir_st.st_mode)) {

    printf("Parametrul trebuie să fie un director valid.\n");

    exit(-1);

  }



  int wr_descriptor = get_wr_descriptor();

  process_directory(wr_descriptor, argv[1]);



  close(wr_descriptor);



  return 0;

}