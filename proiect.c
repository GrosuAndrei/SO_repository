#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <libgen.h>

int pfd1[2]; // capetele pipe-ului dintre procesele fiu
int pfd2[2]; // proces fiu 2 parinte

int contor_global=0;
char name_statistica[100];
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
  if(read(img_descriptor,buffer,18)==-1)
    {
      perror("eroare la citier buffer ");
      exit(-1);
    }
  int* Width=malloc(sizeof(int));
  if(read(img_descriptor,Width,4)==-1)
    {
      perror("eroare la scriere width");
      exit(-1);
    }
  buffer[4]='\0';
  int* Height=malloc(sizeof(int));
  if(read(img_descriptor,Height,4)==-1)
    {
      perror("eroare la scriere height ");
      exit(-1);
    }
  char* result=malloc(100*sizeof(char));
  sprintf(result,"%d %d",*Width,*Height);
  return result;// "width height" concatenate (teoretic)
}

int get_wr_descriptor(char *nume) {
  char folder_name[400];
  sprintf(folder_name,"%s_%s",nume,"statistica.txt");
  int wr_descriptor = creat(folder_name, O_CREAT | S_IRUSR | S_IWUSR | S_IXUSR | O_TRUNC);
  if (wr_descriptor == -1) {
    perror("Fisierul destinatie nu a putut fi deschis\n");
    exit(-1);
  }
  strcpy(name_statistica,folder_name);
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
  if(write(wr_descriptor, result, strlen(result))==-1)
    {
      perror(" access rights error \n");
      exit(-1);
    }
}

// 1*********************************************************************************************

int nr_linii_scrise()
{
   int file_descriptor = open(name_statistica, O_RDONLY);

    if (file_descriptor == -1) {
        perror("eroare la deschiderea fisierului statistica");
        exit(EXIT_FAILURE);
    }
    char buffer[4096];
    int line_count = 0;
    ssize_t read_file;
    while ((read_file = read(file_descriptor, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < read_file; i++) {
            if (buffer[i] == '\n') {
                line_count++;
            }
        }
    }
    if (read_file == -1) {
        perror("eroare la citirea fisierului statistica");
        exit(EXIT_FAILURE);
    }
    return line_count;
}

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
  sprintf(buffer, "nume fisier: %s\ndimensiune: %d\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\n",entry_name, get_total_size(entry_st), get_user_id(entry_st), get_last_mod_time(entry_st));
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
  sprintf(buffer, "nume legatura: %s\ndimensiune legatura: %d\ndimensiune fisier: %d\n", entry_name, get_total_size(entry_st), get_total_size(target_file_st));
  if (write(wr_descriptor, buffer, strlen(buffer)) == -1) {
    perror("first write to symblink file\n");
    exit(-1);
  }
  write_access_rights(wr_descriptor,entry_st);
}

void transfer_data_directory(int wr_descriptor, struct stat entry_st, char* entry_name)
{
  char buffer[1000];
  sprintf(buffer, "nume director: %s\nidentificatorul utilizatorului: %d\n", entry_name, get_user_id(entry_st));
  if (write(wr_descriptor, buffer, strlen(buffer)) == -1) {
    perror("Esec la scriere director");
    exit(-1);
  }
  write_access_rights(wr_descriptor,entry_st);
}

// 2********************************************************************************************

// 3********************************************************************************************

void generare_array_linii_scrise(int* pid, int n, int* nr_linii_arr)
{
  int j;
  for(j = 0; j < n; j++) {
    int status;
    if ( waitpid(pid[j], &status, 0) == -1 ) {
	  perror("Eroare la waitpid");
	  exit(-1);
    }
    if ( WIFEXITED(status) ) {
      int exit_status = WEXITSTATUS(status);
      nr_linii_arr[j] = exit_status;
    }
  }
}

void color_gray(char* entry_img)
{
  char out_img_name[200];
  strncpy(out_img_name,entry_img,strlen(entry_img)-4);
  out_img_name[strlen(out_img_name)-5]='\0';
  strcat(out_img_name,"_modified.bmp");
  int out_img_descriptor = open(out_img_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out_img_descriptor == -1) {
        perror("Error opening output file");
        close(out_img_descriptor);
        exit(-1);
    }

    int input_img_descriptor = open(entry_img, O_RDONLY);
    if (input_img_descriptor == -1) {
        perror("Error opening input file");
        exit(-1);
    }
    char header[54];
    if (read(input_img_descriptor, header, 54) != 54) {
        perror("Error reading input file header");
        close(input_img_descriptor);
        close(out_img_descriptor);
        exit(-1);
    }
    if (write(out_img_descriptor, header, 54) != 54) {
        perror("eroare la scriere in imagine");
        close(input_img_descriptor);
        close(out_img_descriptor);
        exit(-1);
    }
    char pixel[3];
    while (read(input_img_descriptor, pixel, 3) == 3) {
        unsigned char rosu = pixel[2];
        unsigned char verde = pixel[1];
        unsigned char albastru = pixel[0];
        unsigned char gri = 0.299 * rosu + 0.587 * verde + 0.114 * albastru;

        // Scrie valorile calculate in fisierul de iesire
        pixel[0] = gri;
        pixel[1] = gri;
        pixel[2] = gri;
        if (write(out_img_descriptor, pixel, 3) != 3) {
            perror("eroare la modificarea imgainii");
            close(input_img_descriptor);
            close(out_img_descriptor);
            exit(-1);
        }
    }

    // Inchide fisierele
    close(input_img_descriptor);
    close(out_img_descriptor);
}
// 3********************************************************************************************

char* get_data(char* file_name)
{
  int file_descriptor = open(file_name, O_RDONLY);

    if (file_descriptor == -1)
      {
	perror("deschiderea fisierului fara .bmp a esuat");
	exit(-1);
      }

    char *fileContent = (char *)malloc(4096);
    if (fileContent == NULL)
      {
	printf("Eroare la alocarea memoriei.\n");
	close(file_descriptor);
        exit(-1);
      }
    
    ssize_t bytesRead;
    ssize_t totalBytesRead = 0;
    while ((bytesRead = read(file_descriptor, fileContent + totalBytesRead, 4096)) > 0) {
        totalBytesRead += bytesRead;
        fileContent = realloc(fileContent, totalBytesRead + 4096);
        if (fileContent == NULL)
	  {
	    printf("Eroare la realocarea memoriei.\n");
            close(file_descriptor);
	    exit(-1);
	  }
    }

    if (bytesRead == -1) {
        perror("Eroare la citirea fisierului.\n");
        free(fileContent);
        close(file_descriptor);
	exit(-1);
    }
    close(file_descriptor);
    return fileContent;
}

// 4********************************************************************************************
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
      close(img_descriptor);
    } else {
      // Altfel, fișier obișnuit fără extensie .bmp
      transfer_data_not_bmp(wr_descriptor,entry_st,entry_name);
      if(close(pfd1[0]) == -1 || close(pfd2[1]) == -1 || close(pfd2[0]) == -1)
	{
	  perror("eroare la inchiderea capetelor de scriere/citire din pipe proc 1 process_entry");
	  exit(-1);
	}
      char info[1000];
      strcpy(info,get_data(entry_path));
      write(pfd1[1], info, strlen(info)+1);
      
      close(pfd1[1]);
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


void process_directory(char* in_dir_path, char* out_dir_path, char* c) {
  DIR* in_dir;
  struct dirent* entry;
  in_dir = opendir(in_dir_path);
  if (in_dir == NULL) {
    perror("Eroare la deschiderea directorului 1");
    exit(-1);
  }

  DIR* out_dir;
  out_dir=opendir(out_dir_path);
  if(out_dir == NULL)
    {
      perror("Eroare la deschiderea directorului 2");
    }

  int nr_linii;
  int nr_linii_arr[100];
  int wr_descriptor;
  char nume_statistica[500];
  int pid[100];
  char nume_intrari[100][100];
  int i=0;
  
  while ((entry = readdir(in_dir)) != NULL)
    {
      if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
	{
	  if((pid[i]=fork())<0)
	    {
	      perror("Eroare la crearea procesului");
	      exit(-1);
	      }
	  if(pid[i]==0)
	    {
	      nr_linii=0;
	      sprintf(nume_statistica,"%s/%s",out_dir_path, entry->d_name);
	      wr_descriptor = get_wr_descriptor(nume_statistica); 
	      process_entry(wr_descriptor, entry->d_name, in_dir_path);
	      nr_linii=nr_linii_scrise(wr_descriptor);
	      close(wr_descriptor);
	      exit(nr_linii);
	    }
	  char name[500];
	  sprintf(name,"%s/%s",in_dir_path,entry->d_name);
	  struct stat entry_st;
	  if (lstat(name, &entry_st) == -1)
	    {
	      perror("Eroare la stat process_directory");
	      exit(-1);
	    }
	  if (S_ISREG(entry_st.st_mode))
	    {
	      i++;
	       if (strcmp(entry->d_name + strlen(entry->d_name) - 4, ".bmp") == 0)//caz fisier bmp
		 {
		   if((pid[i]=fork())<0)
		     {
		       perror("eroare la crearea procesului adaugator bmp");
		       exit(-1);
		     }
		   if(pid[i]==0)
		     {
		       color_gray(name);
		       exit(0);
		     }
		 }
	       else
		 {
		   if(pipe(pfd1)<0)
		     {
		       perror("Eroare la crearea pipe-ului\n");
		       exit(1);
		     }
	  
		   if( pipe(pfd2) < 0)
		     {
		       perror("eroare la crearea pipe-ului\n");
		       exit(-1);
		     }
		   if((pid[i]=fork())<0)
		  {
		    perror("eroare la crearea procesului adaugator fisier fara extensia bmp");
		    exit(-1);
		  }
		if(pid[i]==0)
		  {
		    if(close(pfd1[1]) == -1 || close(pfd2[0]) == -1)
		      {
			perror("eroare la inchiderea capetelor de scriere/citire din pipe");
			exit(-1); 
		      }
		   
		    if(dup2(pfd2[1], 1) == -1)
		      {
			perror("eroare la redirectare stdout");
			exit(-1);
		      }
		    
		    execlp("/home/andrei/univer/anul_3/SO_proiect/script.sh", "/home/andrei/univer/anul_3/SO_proiect/script.sh", c, NULL);	       
		    perror("Eroare la executia fisierul sh");
		    if(close(pfd2[1]) == -1 || close(pfd1[0]) == -1 || close(pfd1[1]) == -1) {
		      perror("eroare la inchiderea capetelor de scriere/citire din pipe proc 2 executie sh");
		      exit(-1); 
		    }
		    exit(1);
		  }
		char info[1024];
		if(read(pfd2[0], info, sizeof(info)) == -1)
		  {
		    perror("eroare la citire inforamtiei transmise prin pipe");
		    exit(-1);
		  }
		int contor;                      
		sscanf(info, "%d", &contor);
		printf("\nIn fisierul %s exista %d propozitii corecte care contin caracterul %s\n", name ,contor, c);
		contor_global += contor;
		 }
	    }
	  strcpy(nume_intrari[i],entry->d_name);
	  i++;
	}
    }
  generare_array_linii_scrise(pid, i, nr_linii_arr);
  for(int j=0;j<i;j++)
    {
      printf("proces %d evalueaza fisierul: %s scrie %d linii\n", pid[j], nume_intrari[j], nr_linii_arr[j]);
      printf("PID %d terminat",pid[j]);
      perror(" cu codul: ");
    }
  closedir(in_dir);
  closedir(out_dir);
}
// 4****************************************************************************************

int main(int argc, char** argv) {
  if (argc != 4) {
    printf("Usage: %s <director_intrare> <director_iesire> <c>\n", argv[0]);
    exit(-1);
  }

  struct stat entry_dir_st, out_dir_st;
  if (stat(argv[1], &entry_dir_st) == -1 || !S_ISDIR(entry_dir_st.st_mode)) {
    perror("Parametrul entry_dir trebuie să fie un director valid.\n");
    exit(-1);
  }

  if(stat(argv[2], &out_dir_st)==-1 || !S_ISDIR(entry_dir_st.st_mode)) {
    perror("Parametrul out_dir trebuie sa fie un director valid.\n");
    exit(-1);
  }
  

  process_directory(argv[1], argv[2], argv[3]);
  return 0;
}
