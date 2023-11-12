#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <libgen.h>


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
  
char* width_and_height(int img_descriptor) {
  char* buffer = malloc(100 * sizeof(char));
  read(img_descriptor, buffer, 18);

  char* Width = malloc(5 * sizeof(char));
  read(img_descriptor, Width, 4);

  char* Height = malloc(5 * sizeof(char));
  read(img_descriptor, Height, 4);

  char* result = malloc((strlen(Width) + strlen(Height) + 2) * sizeof(char));
  sprintf(result, "%s %s", Width, Height);
  return result;
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

void transfer_data_without_dimensions(int wr_descriptor, struct stat img_st, char* nume_fisier) {
  char* buffer = malloc(100 * sizeof(char));
  sprintf(buffer, "nume fisier: %s", nume_fisier);
  if (write(wr_descriptor, buffer, strlen(buffer)) == -1) {
    perror("Esec la scriere 1");
    exit(-1);
  }

  sprintf(buffer, "\ndimensiune: %d\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s", get_total_size(img_st), get_user_id(img_st), get_last_mod_time(img_st));
  if (write(wr_descriptor, buffer, strlen(buffer)) == -1) {
    perror("Esec la scriere 2");
    exit(-1);
  }

  char buf[10];
  strmode(img_st.st_mode, buf);
  char User_mode[5];
  substrings(0, 2, buf, User_mode);
  char Group_mode[5];
  substrings(3, 5, buf, Group_mode);
  char Other_mode[5];
  substrings(6, 8, buf, Other_mode);
  sprintf(buffer, "\ncontorul de legaturi: %d\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s", get_number_of_hard_links(img_st), User_mode, Group_mode, Other_mode);
  if (write(wr_descriptor, buffer, strlen(buffer)) == -1) {
    perror("Esec la scriere 3");
    exit(-1);
  }

  free(buffer);
}

void transfer_symlink_data(int wr_descriptor, struct stat img_st,  struct stat img_path,  char* nume_legatura) {
  char* buffer = malloc(100 * sizeof(char));
  sprintf(buffer, "nume legatura: %s\ndimensiune legatura: %d\ndimensiune fisier: %d", nume_legatura, get_total_size(img_st), get_total_size(img_path));
  if (write(wr_descriptor, buffer, strlen(buffer)) == -1) {
    perror("Esec la scriere symlink");
    exit(-1);
  }

  char buf[10];
  strmode(img_st.st_mode, buf);
  char User_mode[5];
  substrings(0, 2, buf, User_mode);
  char Group_mode[5];
  substrings(3, 5, buf, Group_mode);
  char Other_mode[5];
  substrings(6, 8, buf, Other_mode);
  sprintf(buffer, "\ndrepturi de acces user legatura: %s\ndrepturi de acces grup legatura: %s\ndrepturi de acces altii legatura: %s", User_mode, Group_mode, Other_mode);
  if (write(wr_descriptor, buffer, strlen(buffer)) == -1) {
    perror("Esec la scriere drepturi symlink");
    exit(-1);
  }

  free(buffer);
}

void transfer_directory_data(int wr_descriptor, struct stat img_st, char* nume_director) {
  char* buffer = malloc(100 * sizeof(char));
  sprintf(buffer, "nume director: %s\nidentificatorul utilizatorului: %d", nume_director, get_user_id(img_st));
  if (write(wr_descriptor, buffer, strlen(buffer)) == -1) {
    perror("Esec la scriere director");
    exit(-1);
  }

  char buf[10];
  strmode(img_st.st_mode, buf);
  char User_mode[5];
  substrings(0, 2, buf, User_mode);
  char Group_mode[5];
  substrings(3, 5, buf, Group_mode);
  char Other_mode[5];
  substrings(6, 8, buf, Other_mode);
  sprintf(buffer, "\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s", User_mode, Group_mode, Other_mode);
  if (write(wr_descriptor, buffer, strlen(buffer)) == -1) {
    perror("Esec la scriere drepturi director");
    exit(-1);
  }

  free(buffer);
}

struct stat get_status(char* nume)
{
  struct stat st;
  if(stat(nume,&st)==-1)
    {
      perror("”Usage ./program <fisier_intrare>”.\n");
      exit(-1);
    }
  return st;
}

void caz_fisier_bmp(char** argv)
{
  struct stat img_st=status(argv);
  int img_descriptor=get_img_descriptor(argv[1]);
  int wr_descriptor=get_wr_descriptor();
  transfer_data(wr_descriptor,img_st,img_descriptor,argv[1]);
}

void caz_fisier_nu_bmp(char** argv)
{
  struct stat img_st=get_status(argv[1]);
  int wr_descriptor=get_wr_descriptor();
  transfer_data_without_dimensions(wr_descriptor,img_st,argv[1]);
}

void caz_legatura_simbolica(char** argv, char* entry_path, char* entry_name)
{
  struct stat img_st=get_status(entry_name);
  struct stat img_path=get_status(entry_path);
  int wr_descriptor=get_wr_descriptor();
  transfer_symlink_data(wr_descriptor,img_st,img_path,entry_name);
}


void caz_director(char** argv, char* entry_name)
{
  struct stat img_st=status(argv);
  int wr_descriptor=get_wr_descriptor();
  transfer_directory_data(wr_descriptor,img_st, entry_name);
}



void process_entry(int wr_descriptor, char* entry_name, char* dir_path, char** argv) {
  char entry_path[100];
  sprintf(entry_path, "%s/%s", dir_path, entry_name);

  struct stat entry_st;
  if (lstat(entry_path, &entry_st) == -1) {
    perror("Eroare la stat-ul intrării");
    exit(-1);
  }

  if (S_ISREG(entry_st.st_mode)) {
    if (strstr(entry_name, ".bmp") != NULL) {
      // Fișier cu extensia .bmp
      caz_fisier_bmp(argv);
    } else {
      // Altfel, fișier obișnuit fără extensie .bmp
      caz_fisier_nu_bmp(argv);
    }
  } else if (S_ISLNK(entry_st.st_mode)) {
    // Legătură simbolică
    caz_legatura_simbolica(argv, entry_path, entry_name);
  } else if (S_ISDIR(entry_st.st_mode)) {
    // Director
    caz_director(argv, entry_name);
  }
}

void process_directory(int wr_descriptor, char* dir_path, char** argv) {
  DIR* dir;
  struct dirent* entry;

  dir = opendir(dir_path);
  if (dir == NULL) {
    perror("Eroare la deschiderea directorului");
    exit(-1);
  }

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
      process_entry(wr_descriptor, entry->d_name, dir_path, argv);
    }
  }

  closedir(dir);
}

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
  process_directory(wr_descriptor, argv[1], argv);

  close(wr_descriptor);

  return 0;
}
