#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <ctype.h>
#include <sys/wait.h>

#define FIFO_NAME_RESP "RESP_PIPE_30171"
#define FIFO_NAME_REQ "REQ_PIPE_30171"

unsigned int shm_size;
volatile char *data = NULL;
volatile char *file_data = NULL;
int file_size;


typedef struct {
    char magic[3];
    int version;
    int header_size;
    int nr_sections;
    struct {
        char section_name[9];
        int section_type;
        int section_offset;
        int section_size;
        int section_blocks;
    } section_header[11];
} SFHeader;

SFHeader s;
int parseSf(SFHeader *s) {
    // magic - AM
    s->magic[0] = file_data[0];
    s->magic[1] = file_data[1];
    s->magic[2] = 0;
    if (s->magic[0] != 'A' || s->magic[1] != 'M') {
            printf("ERROR\nwrong magic");
        return 1;
    }
     // Header size (2 bytes)
    s->header_size = *(unsigned short*)&file_data[2];
    //printf("%d \n\n", s->header_size);
    
    // Version (2 bytes)
    s->version = *(unsigned short*)&file_data[4];
    //printf("%d \n\n", s->version);
    if (s->version < 68 || s->version > 88) {
        
            printf("ERROR\nwrong version");
        return 1;
    }
    // section number
    s->nr_sections = file_data[6];
    if (s->nr_sections != 2)
        if (s->nr_sections < 4 || s->nr_sections > 11) {
            
                printf("ERROR\nwrong sect_nr");
            return 1;
        }

    // Read sections
    for (int i = 0; i < s->nr_sections; i++) {
        int base_index = 7 + i * 17;
        memcpy(s->section_header[i].section_name,(const char *)(file_data + base_index), 8);
        s->section_header[i].section_name[9] = '\0';

        s->section_header[i].section_type = file_data[base_index + 8];
        memcpy(&s->section_header[i].section_offset, (const int*) (file_data + base_index+9), sizeof(int));
        memcpy(&s->section_header[i].section_size, (const int*) (file_data + base_index+13), sizeof(int));

  
	if (s->section_header[i].section_type != 49 && s->section_header[i].section_type != 50) {
		    printf("ERROR\nwrong sect_types");
		return 1;
        }
    }

        // show success
        printf("SUCCESS\n");
        printf("magic=%s\n", s->magic);
        printf("header_size=%x\n", s->header_size);
        
        printf("version=%x\n", s->version);
        printf("nr_sections=%x\n", s->nr_sections);
        for (int i = 0; i < s->nr_sections; i++) {
            printf("section%d: name %s type %x  size %d offset %x\n", i + 1, s->section_header[i].section_name, s->section_header[i].section_type, s->section_header[i].section_size, s->section_header[i].section_offset);
        }

    return 0;
}


int calc(unsigned int logical_offset, unsigned int no_of_bytes) {
	int blocks = 0;
	int logicalOffset = 0; //iterator for what we consider logical offset
	int iterator =0;
 
		 for (int i = 0; i < s.nr_sections; i++) {
		  
			int sectionSize = s.section_header[i].section_size;
			
			int sectionIterator=0; // iterator for the section content
			while (sectionIterator < sectionSize) {
				if (sectionIterator%4096 ==0){
					//printf("\nBlock %d: \n", blocks);
					logicalOffset = 4096 * blocks; //begining of each block 
					blocks ++; 
				}
			if (logicalOffset >= logical_offset && logicalOffset < logical_offset+no_of_bytes){
				data[iterator] = file_data[s.section_header[i].section_offset + sectionIterator];
				iterator++; //iterator for giving wanted bytes
			}
			    sectionIterator++;
			    logicalOffset++;
			}
			//printf("\n");
		 }
		if (iterator < no_of_bytes){
			return 1; //nu am reusit sa parcurgem nr cerut de bytes
		}
		else 
			return 0;
	
	
	
}
	


int main(){
unlink(FIFO_NAME_RESP);
int frq = -1;
int frsp = -1;
//create resp pipe
if(mkfifo(FIFO_NAME_RESP, 0600) != 0) {
        printf("cannot create the response pipe");
        return 1;
}
//open request pipe for reading
frq = open(FIFO_NAME_REQ, O_RDONLY);
if(frq == -1) {
	printf("cannot open the request pipe");
        return 1;
}
//open response pipe for writing
frsp = open(FIFO_NAME_RESP, O_WRONLY);
if(frsp == -1) {
	printf("cannot open the response pipe");
        return 1;
}

write(frsp, "START#", 6);

char option[250];
while (1){	
	//read option
	char c;
	int i=0;
	do{
		read(frq, &c, 1);
		option[i] = c;
		i++;
	}while (c!='#');
	option[i] = 0;
	
	
	if (strncmp(option,"ECHO",4) == 0){
		 write(frsp, "ECHO#", 5);
		 write(frsp, "VARIANT#", 8);
		 int x = 30171;
		 write(frsp, &x, sizeof(int));
		
	}

	if (strncmp(option,"EXIT",4) == 0){
		close(frq);
		close(frsp);
		unlink(FIFO_NAME_RESP);
		return 0;
	}
	if (strncmp(option,"CREATE_SHM",10) == 0){
		write(frsp, "CREATE_SHM#", 11);
		
		read(frq,&shm_size, sizeof(unsigned int));
		int shm_fd = shm_open("/BxCwwNH", O_CREAT | O_RDWR, 0664);
		if (shm_fd < 0) {
		    write(frsp, "ERROR#", 6);
		    return 1;
        	} else {
        		ftruncate(shm_fd, shm_size);
        		data = (volatile char*)mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
			if (data == (void*)-1) {
				write(frsp, "ERROR#", 6);
			    	return 1;
			} else {
				write(frsp, "SUCCESS#", 8);
			}
        	}
		
	}
	if (strncmp(option, "WRITE_TO_SHM", 12) == 0) {
            write(frsp, "WRITE_TO_SHM#", 13);
            unsigned int offset, value;
            read(frq, &offset, sizeof(unsigned int));
            read(frq, &value, sizeof(unsigned int));
            
            if (offset < 0 || offset >= shm_size || (offset + sizeof(unsigned int)) > shm_size) {
                write(frsp, "ERROR#", 6);
            } else {
                *(unsigned int *)(data + offset) =value;
                write(frsp, "SUCCESS#", 8);
            }
        }
        
        
	if (strncmp(option,"MAP_FILE",8) == 0){
		write(frsp, "MAP_FILE#", 9);
		char file_name[250];
		int i =0;
		do{
			read(frq, &c, 1);
			file_name[i] = c;
			i++;
		}while (c!='#');
		
		file_name[i-1] = '\0';
		
		
		int fd = open(file_name, O_RDONLY);
		if(fd == -1) {
			write(frsp, "ERROR#", 6);
			return 1;
		}
		file_size = lseek(fd, 0, SEEK_END);
		file_data = (char*)mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
		if(file_data == (void*)-1) {
			write(frsp, "ERROR#", 6);
			close(fd);
			return 1;
		}
		//for (int i=0;i<=file_size;i++){
		//printf("%c ",file_data[i]);
		//}
		parseSf(&s); //!!!only header initialisation
		write(frsp, "SUCCESS#", 8);
	}
	
	
	 if (strncmp(option, "READ_FROM_FILE_OFFSET", 21) == 0) {
            write(frsp, "READ_FROM_FILE_OFFSET#", 22);
            unsigned int offset, no_of_bytes;
            read(frq, &offset, sizeof(unsigned int));
            read(frq, &no_of_bytes, sizeof(unsigned int));
            
	    if (file_data == NULL) {
		    write(frsp, "ERROR#", 6);
	    } else if (offset + no_of_bytes > file_size) {
	    	    write(frsp, "ERROR#", 6);
	    }else {
	    
		    for (int i=0; i < no_of_bytes; i++) {
		            data[i] = file_data[i+offset];
			    //printf("%c ", data[i]);
		    }
		    write(frsp, "SUCCESS#", 8);
            } 
         }
	
	
	if (strncmp(option, "READ_FROM_FILE_SECTION", 22) == 0) {
            write(frsp, "READ_FROM_FILE_SECTION#", 23);
            unsigned int section_no = 0, offset = 0, no_of_bytes = 0;
            read(frq, &section_no, sizeof(unsigned int)); //7
            read(frq, &offset, sizeof(unsigned int)); //0
            read(frq, &no_of_bytes, sizeof(unsigned int)); //100
            
            //printf("section nb: %d offset: %d bytes: %d\n\n", section_no, offset,no_of_bytes);
  
	    if (s.nr_sections < section_no) {
	    //printf("wanted section : %d nr_sections: %d\n\n", section_no, s.nr_sections);
	    	write(frsp, "ERROR#", 6);
	
	    } else if (s.section_header[section_no-1].section_size < no_of_bytes) {
	    //printf("wanted bytes : %d section size %d\n\n", no_of_bytes, s.section_header[section_no-1].section_size);
	   	 write(frsp, "ERROR#", 6);
	    } else {
	    
	    int starting_offset = s.section_header[section_no-1].section_offset + offset;
	    for (int i=0;i<no_of_bytes;i++){
	    	data[i] = file_data[i+starting_offset];
	    }
	     write(frsp, "SUCCESS#", 8);
	    }

           
        }
	
	
	 if (strncmp(option, "READ_FROM_LOGICAL_SPACE_OFFSET", 30) == 0) {
            unsigned int logical_offset, no_of_bytes;
            read(frq, &logical_offset, sizeof(unsigned int));
            read(frq, &no_of_bytes, sizeof(unsigned int));

            if (calc(logical_offset, no_of_bytes) == 0){
             write(frsp, "READ_FROM_LOGICAL_SPACE_OFFSET#", 31);
          	 write(frsp, "SUCCESS#", 8);
           } else {
             write(frsp, "READ_FROM_LOGICAL_SPACE_OFFSET#", 31);
                 write(frsp, "ERROR#", 6);
           
            }

        }
	
}
}
