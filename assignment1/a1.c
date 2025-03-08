#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>


typedef struct {
    char magic[3];
    int version;
    int nr_sections;
    struct {
        char section_name[9];
        int section_type;
        int section_offset;
        int section_size;
    } section_header[11];
} SFHeader;

int parseSf(int fd, SFHeader *s, int x) {
    lseek(fd, 0, SEEK_SET);
    // magic - AM
    read(fd, s->magic, 2);
    s->magic[2] = 0;
    if (s->magic[0] != 'A' || s->magic[1] != 'M') {
        if (x == 1)
            printf("ERROR\nwrong magic");
        return 1;
    }
    // header size
    //read(fd, &s.header_size, 2);

    // version
    s->version = 0;
    lseek(fd, 4, SEEK_SET);
    read(fd, &s->version, 2);
    if (s->version < 68 || s->version > 88) {
        if (x == 1)
            printf("ERROR\nwrong version");
        return 1;
    }

    // section number
    s->nr_sections = 0;
    read(fd, &s->nr_sections, 1);
    if (s->nr_sections != 2)
        if (s->nr_sections < 4 || s->nr_sections > 11) {
            if (x == 1)
                printf("ERROR\nwrong sect_nr");
            return 1;
        }

    // Read sections
    for (int i = 0; i < s->nr_sections; i++) {
        lseek(fd, 7 + i * 17, SEEK_SET);
        read(fd, &s->section_header[i].section_name, 8);
        s->section_header[i].section_name[8] = 0;
        s->section_header[i].section_type = 0;
        s->section_header[i].section_offset = 0;
        s->section_header[i].section_size = 0;
        read(fd, &s->section_header[i].section_type, 1);
        read(fd, &s->section_header[i].section_offset, 4);
        read(fd, &s->section_header[i].section_size, 4);

        if (s->section_header[i].section_type != 49 && s->section_header[i].section_type != 50) {
		if (x == 1)
		    printf("ERROR\nwrong sect_types");
		return 1;
        }
    }

    if (x == 1) {
        // show success
        printf("SUCCESS\n");
        printf("version=%d\n", s->version);
        printf("nr_sections=%d\n", s->nr_sections);
        for (int i = 0; i < s->nr_sections; i++) {
            printf("section%d: %s %d %d\n", i + 1, s->section_header[i].section_name, s->section_header[i].section_type, s->section_header[i].section_size);
        }
    }
    return 0;
}

void searchLine(int fd, int sect_nr, int line_nr, SFHeader s) {
   parseSf(fd, &s, 0); //parse works now
    int offset = s.section_header[sect_nr-1].section_offset + s.section_header[sect_nr-1].section_size -1;
    char val;
    int line_count = 1;
    while (line_count < line_nr && offset >0) {
        lseek(fd, offset, SEEK_SET);
        read(fd, &val, 1);
           
        if (val == '\n') {
            line_count++;
        }
	offset--;
    }

    if (line_count > line_nr) {
       	printf("ERROR\nline not found");
       	return;
   }

//reading line
	//char line[10000]; 
	//int index = 0;
	printf("SUCCESS\n");
	while (offset >= 0) {
		lseek(fd, offset--, SEEK_SET);
		if (read(fd, &val, 1) != 1) {
			printf("ERROR\ninvalid directory path");
	    		return;
		}
		if (val == 0 || val == '\n') {
			break; // end of line
		}
		//line[index++] = val;
		//afisare directa, works for large lines
		printf("%c",val);
	}
	//line[index] = '\0';
	//printf("SUCCESS\n%s\n", line);
}

//5.2.3.2 lab listRec
void listDir(const char* path, int rec){

	struct dirent *entry= NULL;
	struct stat statbuf;


	char fullPath[512];
	DIR *dir=NULL;
	dir= opendir(path);

	if (dir == NULL){
	    printf("ERROR\ninvalid directory path");
	    return;
	}
	while ((entry = readdir(dir)) != NULL){
	    if (strcmp(entry -> d_name, "." )!= 0  && strcmp(entry->d_name, "..") != 0){
		snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
		if (lstat(fullPath, &statbuf) == 0){
		    printf("%s\n", fullPath);
		    if (S_ISDIR(statbuf.st_mode)){
			if (rec ==1){
		        	listDir(fullPath, rec);
		        }
		    }
		}
	    }
	}

	closedir(dir);
}    



void listDirFilter(const char* path, int rec, int filterType, char *filter){
struct dirent *entry= NULL;
struct stat statbuf;

char fullPath[512];
DIR *dir=NULL;
dir= opendir(path);

if (dir == NULL){
    printf("ERROR\ninvalid directory path");
    return;
}
while ((entry = readdir(dir)) != NULL){
    if (strcmp(entry -> d_name, "." )!= 0  && strcmp(entry->d_name, "..") != 0){
    snprintf(fullPath, 512, "%s/%s", path, entry->d_name);

        if (lstat(fullPath, &statbuf) == 0){
        	//filter ptr name_ends_with
        	if (filterType == 1) { 
		    	int startPos=strlen(entry->d_name)-strlen(filter); //amplasat unde ar trebui sa inceapa textul extensie
		    	if (strcmp(entry->d_name+startPos, filter) == 0) {
		  	      printf("%s\n", fullPath);
		  	  }
		}
		//filter ptr has_perm_execute
		if (filterType == 2) {	 
        		if(statbuf.st_mode & S_IXUSR) //S_IXUSR = executabil ptr file owner
        			printf("%s\n", fullPath);
        	}	
		if (S_ISDIR(statbuf.st_mode)){
			if (rec==1){
		        	listDirFilter(fullPath,rec, filterType, filter);
		        }       
		}
    		
    	}
    }
}

closedir(dir);
}    

void listDirFindAll(const char* path,  SFHeader s){

	struct dirent *entry= NULL;
	struct stat statbuf;


	char fullPath[512];
	DIR *dir=NULL;
	dir= opendir(path);

	if (dir == NULL){
	    printf("ERROR\ninvalid directory path");
	    return;
	}
	
	while ((entry = readdir(dir)) != NULL){
	    if (strcmp(entry -> d_name, "." )!= 0  && strcmp(entry->d_name, "..") != 0){
		snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
		if (lstat(fullPath, &statbuf) == 0){
			int fd = open(fullPath, O_RDONLY);
				if (fd == -1) {
				    perror("could not open input file");
				    return;
			}
		    if (parseSf(fd, &s, 0) == 0){ //sf valid
		    	int ct = 0;
		    	for (int i=0; i<s.nr_sections; i++)
				if (s.section_header[i].section_type == 49)
					ct++;
			if (ct >=3)
				printf("%s\n", fullPath);
		    }
		    
		    if (S_ISDIR(statbuf.st_mode)){
		        listDirFindAll(fullPath, s);
		    }
		}
	    }
	}

	closedir(dir);
}    

    
    
int main(int argc, char **argv) {
SFHeader s;
char *file_path = NULL;
char *filter = NULL;
int sect_nr =0;
int line_nr = 0;
    if (argc >= 2) {
        if (strcmp(argv[1], "variant") == 0){
            printf("30171\n");
        } else if (strcmp(argv[1], "parse") == 0) {
        
		if (argc >= 3) 
		{
		   	 for (int i = 1; i < argc; i++) {
		       		 if (strncmp(argv[i], "path=", 5) == 0) {
			  		  file_path = argv[i] + 5; // skip "path="
			   		 break;
			   	}
			 }
			 
			 if (file_path != NULL) {
			int fd = open(file_path, O_RDONLY);
			if (fd == -1) {
			    perror("could not open input file");
			    return 1;
			}
			parseSf(fd,&s,1);
			}
		}
        }else if (strcmp(argv[1], "extract") == 0) {
  
        	if (argc > 4) {

			for (int i = 1; i < argc; i++) {
				//line nr
		       		 if (strncmp(argv[i], "line=", 5) == 0) {
			  		  line_nr = atoi(argv[i] + 5); //skip "line="
			   	}
			   	//sect nr
			   	if (strncmp(argv[i], "section=", 8) == 0) {
			  		  sect_nr = atoi(argv[i] + 8); //"section="
			   	}
			   	//file name
			   	 if (strncmp(argv[i], "path=", 5) == 0) {
			  		  file_path = argv[i] + 5; // skip "path="
			   	}
			 }
	 
			if (file_path != NULL) {
				int fd = open(file_path, O_RDONLY);
				if (fd == -1) {
				    perror("could not open input file");
				    return 1;
				}
				searchLine(fd, sect_nr, line_nr,s); 
			}
		}
        }else if (strcmp(argv[1], "list") == 0) {
		if (argc >= 3) 
		{	
			int rec = 0;
			int filterType = 0;
		   	 for (int i = 1; i < argc; i++) {
		   	 	//recursive or not
		       		 if (strncmp(argv[i], "recursive", 9) == 0) {
			  		 rec = 1;
			   	}
			   	//file name
			   	 if (strncmp(argv[i], "path=", 5) == 0) {
			  		  file_path = argv[i] + 5; // skip "path="
			   	}
			   	//filtering options
			   	if (strncmp(argv[i], "name_ends_with=", 15) == 0) {
			  		  filter = argv[i] + 15; // skip "name_ends_with="
			  		  filterType = 1;
			   	}
			   	if (strncmp(argv[i], "has_perm_execute", 16) == 0) {
			  		  filterType = 2;
			   	}
			 }
			 
			printf("SUCCESS\n");
			if (filterType == 1 || filterType == 2){
				listDirFilter(file_path,rec, filterType, filter);
			}else{
			    listDir(file_path,rec);	
			}
		}
    	} else if (strcmp(argv[1], "findall") == 0) {
		if (argc >= 3) 
		{	
		   	 for (int i = 1; i < argc; i++) {
			   	//file name
			   	 if (strncmp(argv[i], "path=", 5) == 0) {
			  		  file_path = argv[i] + 5; // skip "path="
			   	}
			
			 }
			 printf("SUCCESS\n");
			 listDirFindAll(file_path, s);
			}
		}
    }
    return 0;
}
