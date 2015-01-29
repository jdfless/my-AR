/*
Jonathan Flessner
flessnej@onid.oregonstate.edu
CS311-400
Homework 4.3 - myar.c
28 April 2014
*/
/*
The -w command extra credit is attempted here.  The usage is ./myar -w <archive> <seconds>.
The program will then wait (sleep) for the designated number of seconds.  Upon resume, it
will compare the state of the current directory before it slept, to the state of the directory
now.  If any files have been modified, the files will be added to the archive.
*/
/*
Extra credit attempted via 3.6.18. Identical files are not added to the archive.
Identical means files of same name, modification date, uid, gid, permissions, and size.
If a file with the same name, that is NOT identical is added. The first found file
with the same name (that isn't identical) is removed, and the other file is added
to the end of the archive.
//NOTE: This makes the functionality of myar q different from ar q by design. 
Some automated tests may fail if directly comparing myar q to ar q with multiple adds.
Please be aware of this.
Also, due to the specific directions in w that it may result in multiple copies of the
same file, this functionality by design does NOT effect w.
*/

#include <ar.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>


void usage(char **argv);
void tflag(char **argv);
void vflag(char **argv);
void qflag(int memNum, char **argv);
void fillHeader(char *mem, char *arch);
void fillBody(char *mem, char *arch);
void xflag(int memNum, char **argv);
void extract(char *mem, char *arch);
void dflag(int memNum, char **argv);
int compare(const void * a, const void * b); //for qsort
void Aflag(int memNum, char **argv);
int deleteOld(char *mem, char *arch);
void wflag(int memNum, char **argv);

int main (int argc, char **argv){
	//test too few command line args
	if (argc < 3){
		usage(argv);
        exit(EXIT_FAILURE);
	}
	//test key
	if(strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "t") == 0)
		tflag(argv);
	else if(strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "v") == 0)
		vflag(argv);
	else if(strcmp(argv[1], "-q") == 0 || strcmp(argv[1], "q") == 0)
		qflag(argc - 3, argv); 	//argc - 3 gives number of members
	else if(strcmp(argv[1], "-x") == 0 || strcmp(argv[1], "x") == 0)
		xflag(argc - 3, argv);
	else if(strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "d") == 0)
		dflag(argc - 3, argv);
	else if(strcmp(argv[1], "-A") == 0 || strcmp(argv[1], "A") == 0)
		Aflag(argc - 3, argv);
	else if(strcmp(argv[1], "-w") == 0 || strcmp(argv[1], "w") == 0)
		wflag(argc - 3, argv);
	else {
		printf("Error, key not recognized.\n");
		usage(argv);
		exit(EXIT_FAILURE);
	}

	return 0;
}
void usage(char **argv){
	fprintf(stderr, "Usage: %s <key> <archive-file> [member files...]\n", argv[0]);
}

void tflag(char **argv){
	//set archive name
	char *archName = argv[2];
	//exit if archive doesn't exist
	if (access(archName, F_OK) == -1){
		printf("ERROR: Archive does not exist or cannot be accessed.\n");
		exit(EXIT_FAILURE);
	}
	//make sure not bad archive file
	else{
		char tms[9] = {0};						//to test magic string
		int fd = open(archName, O_RDONLY, 0);	//open file
		read(fd, tms, 8);						//read magic string
		if(strcmp(tms, ARMAG) != 0){
			printf("ERROR: '%s' is non-standard archive.\n", archName);
			exit(EXIT_FAILURE);
		}
		close(fd);
	}
	//-t does not take member arguments. they are ignored.
	int fd = open(archName, O_RDONLY, 0); //open file
	
	struct stat st;  //use stat to get archive file size
	int size = 0; 	//will hold archive file size
	if(stat(archName, &st) == 0)
		size = st.st_size; 			//size = archName file size
	
	struct ar_hdr ah; //initialize struct ar_hdr - see linux man page ar(3)
	char buf[sizeof(ah.ar_name) + 1]; //size is 16
	char sbuf[sizeof(ah.ar_size) + 1]; //size is 10
	int fsize = 0; //to store file member size
	int hsize = sizeof(ah.ar_date) + sizeof(ah.ar_uid) + sizeof(ah.ar_gid) + sizeof(ah.ar_mode); //rest of header size - 32
	
	int pos = lseek(fd, 8, 0); //set pos to end of magic string
	while(pos < size){
		//magic string of 8 bytes already skipped
		//print name without terminating slash
		int t = 0;
		while(t < sizeof(ah.ar_name)){
			read(fd, &buf[t], 1);
			if(buf[t] == '/'){
				buf[t] = 0;		//delete terminating slash
				t++;
				pos = lseek(fd, 16 - t, 1);
				printf("%s\n", buf);	//prints name and newline
				break;
			}
			t++;
		}
		//name printed
		pos = lseek(fd, hsize, 1);			//seek to file size
		read(fd, sbuf, sizeof(ah.ar_size)); //read file size
		fsize = atoi(sbuf);					//set file size as int with atoi	
		//printf("file size: %d", fsize);   //for showing file size if needed
		fsize = fsize + sizeof(ah.ar_fmag); //add trailer string to file size
		if(fsize % 2 != 0)					//account for odd sized files
			fsize += 1;						//add one to file size if odd
		pos = lseek(fd, fsize, 1); 			//seek to next file member name
		memset(&buf[0], 0, sizeof(buf)); 	//clear buffer
	}
	close(fd);
}

void vflag(char **argv){
	//set archive name
	char *archName = argv[2];
	//exit if archive doesn't exist
	if (access(archName, F_OK) == -1){
		printf("ERROR: Archive does not exist or cannot be accessed.\n");
		exit(EXIT_FAILURE);
	}
	//make sure not bad archive file
	else{
		char tms[9] = {0};						//to test magic string
		int fd = open(archName, O_RDONLY, 0);	//open file
		read(fd, tms, 8);						//read magic string
		if(strcmp(tms, ARMAG) != 0){
			printf("ERROR: '%s' is non-standard archive.\n", archName);
			exit(EXIT_FAILURE);
		}
		close(fd);
	}
	//-v does not take member arguments. they are ignored.
	int fd = open(archName, O_RDONLY, 0); 	//open file

	struct stat st;  						//use stat to get archive file size
	int size = 0; 							//will hold archive file size
	if(stat(archName, &st) == 0)
		size = st.st_size; 					//size = archName file size
	
	struct ar_hdr ah; 						//initialize struct ar_hdr - see linux man page ar(3)
	char mode[10] = {0}; 					//for printing permissions
	char octMode[9] = {0}; 					//for reading permissions
	char octCon[4] = {0};  					//for converting from octal to symbolic
	int uid = 0;
	int gid = 0;
	int fsize = 0;
	int seconds = 0; 						//raw time 
	time_t cTime = 0;						//used for the ctime function
	char fullDate[27] = {0};				//includes day of week
	char dayDate[13] = {0};					//without day of week
	char yearDate[6] = {0};					//just end of date, space plus year
	char date[18] = {0};					//in proper ar format after concatenating
	char name[sizeof(ah.ar_name) + 1] = {0};//size of ar_name is 16
	char temp[sizeof(ah) + 1] = {0}; 		//size of ah is 60, plenty big, used for some reading and atoi conversions
	
	int pos = lseek(fd, 8, 0); 					//set pos to end of magic string
	//loop over each file member header
	while(pos < size){
		int t = 0;							//print name without slash
		while(t < sizeof(ah.ar_name)){
			read(fd, &name[t], 1);			//read byte by byte
			if(name[t] == '/'){				//check if end of name
				name[t] = 0;				//delete terminating slash
				t++;						//inc iterator
				pos = lseek(fd, 16 - t, 1);	//move to date
				break;						//full name stored. break.
			}
			t++;
		}
		//read(fd, name, sizeof(ah.ar_name)); //put file name in name
		read(fd, temp, sizeof(ah.ar_date));
		seconds = atoi(temp);				//convert string to int seconds
		cTime = (time_t)seconds;			//cast seconds to time_t
		strcpy(fullDate, ctime(&cTime));	//use cTime to get human date, copy to fullDate
		strncpy(dayDate, fullDate + 4, 12);	//copy from month until end of minutes, drop weekday
		strncpy(yearDate, fullDate + 19, 5);//copy space and year
		strcpy(date, dayDate);
		strcat(date, yearDate);
		date[17] = '\0';					//add null terminating char
		//printf("Time is:%s", date);
		memset(&temp[0], 0, sizeof(temp)); 	//clear temp
		read(fd, temp, sizeof(ah.ar_uid));	//read uid
		uid = atoi(temp);					//store uid as int
		read(fd, temp, sizeof(ah.ar_gid));	//read gid
		gid = atoi(temp);					//store gid as int
		read(fd, octMode, sizeof(ah.ar_mode)); //read member mode
		strncpy(octCon, octMode + 3, 3); 	//used to convert file perms - keep only relevant octal notation
		//convert from octal to symbolic permission notation
		int j = 0;
		int i = 0;
		for(i = 0; i < 3; i++){
			if(octCon[i] == '7'){
				mode[j] = 'r';
				mode[j+1] = 'w';
				mode[j+2] = 'x';
				j += 3;
			}
			else if(octCon[i] == '6'){
				mode[j] = 'r';
				mode[j+1] = 'w';
				mode[j+2] = '-';
				j += 3;
			}
			else if(octCon[i] == '5'){
				mode[j] = 'r';
				mode[j+1] = '-';
				mode[j+2] = 'x';
				j += 3;
			}
			else if(octCon[i] == '4'){
				mode[j] = 'r';
				mode[j+1] = '-';
				mode[j+2] = '-';
				j += 3;
			}
			else if(octCon[i] == '3'){
				mode[j] = '-';
				mode[j+1] = 'w';
				mode[j+2] = 'x';
				j += 3;
			}
			else if(octCon[i] == '2'){
				mode[j] = '-';
				mode[j+1] = 'w';
				mode[j+2] = '-';
				j += 3;
			}
			else if(octCon[i] == '1'){
				mode[j] = '-';
				mode[j+1] = '-';
				mode[j+2] = 'x';
				j += 3;
			}
			else{
				mode[j] = '-';
				mode[j+1] = '-';
				mode[j+2] = '-';
				j += 3;
			}
		}
		read(fd, temp, sizeof(ah.ar_size));	//read member file size
		fsize = atoi(temp);					//hold member file size
		printf("%s %d/%d", mode, uid, gid); //print in ar -tv <archivefile> format
		printf("%7d %s %s\n", fsize, date, name);
		fsize = fsize + sizeof(ah.ar_fmag); //add trailer string to file size
		if(fsize % 2 != 0)					//account for odd sized files
			fsize += 1;						//add one to file size if odd
		pos = lseek(fd, fsize, 1); 			//seek to next file member name
	}
	close(fd);
}

void qflag(int memNum, char **argv){
	//variables
	int del = 0;
	char *archName = argv[2];	//set archName
	int fd = 0;					//init file descriptor
	//case for no members
	if(memNum == 0){
		//check if arch file exists, create if doesn't
		if (access(archName, F_OK) == -1){ //file doesn't exist
			if((fd = creat(archName, 0666)) == -1){
				printf("ERROR: Can't create %s\n", archName);
				exit(EXIT_FAILURE);
			}
			fd = open(archName, O_RDWR, 0); //open file
			write(fd, ARMAG, SARMAG);		//write ar magic string
			close(fd);						//close file  
		}
		else{
			exit(EXIT_SUCCESS); //file exists, no members to add
		}
	}
	//case for memNum > 0
	//check if arch file exists, create if doesn't
	if (access(archName, F_OK) == -1){ //file doesn't exist
		if((fd = creat(archName, 0666)) == -1){
			printf("ERROR: Can't create %s\n", archName);
			exit(EXIT_FAILURE);
		}
		fd = open(archName, O_RDWR, 0); //open file
		write(fd, ARMAG, SARMAG);		//write ar magic string
		close(fd);						//close file 
	}
	//archive already exists, ensure good archive file
	else{
		char tms[9] = {0};					//to test magic string
		fd = open(archName, O_RDONLY, 0);	//open file
		read(fd, tms, 8);					//read magic string
		if(strcmp(tms, ARMAG) != 0){
			printf("ERROR: '%s' is non-standard archive.\n", archName);
			exit(EXIT_FAILURE);
		}
		close(fd);
	}
	int c = 0; //count of members appended
	int i = 3; //iterator for argv
	for(c = 0; c < memNum; c++){
		char *mem = argv[i]; 	//set member
		//check if member exists
		if(access(mem, F_OK) == -1){
			printf("myar: %s: File does not exist.\n", mem);
			exit(EXIT_FAILURE);
		}
		del = deleteOld(mem, archName);		//returns 0 if need to add
		if(del == 1){						//returns 1 if no add
			i++;							//if 1, inc i
			printf("File: '%s' has identical copy in archive. File not added.\n", mem);
			continue;						//continue w/o adding
		}
		fillHeader(mem, archName);
		fillBody(mem, archName);
		if(del == 2){
			printf("Old version of '%s' removed from archive. New version added.\n", mem);
		}
		i++;
	}
}

void fillHeader(char *mem, char *arch){
	struct stat st;
	if (stat(mem, &st) == -1){
		printf("ERROR: Stat failed.");
		exit(EXIT_FAILURE);
	}
	char term[1] = {'/'};	//terminating char for file member name
	char fill[1] = {' '};	//whitespace for filling headers 
	int fd = open(arch, O_RDWR, 0);	//open arch file
	int pos = 0;
	pos = lseek(fd, 0, 2);		//seek to end of arch file
	if(strlen(mem) + 1 > 16){
		printf("ERROR: File name too long.");
		exit(EXIT_FAILURE);
	}
	//write name
	//printf("Mem=%s|\n", mem);
	write(fd, mem, strlen(mem));
	//printf("Sizeof=%d\n", strlen(mem));
	write(fd, term, 1);
	int sxtn = strlen(mem) + 1;
	while(sxtn < 16){
		write(fd, fill, 1);
		sxtn++;
	}
	//write date
	time_t modTime = st.st_mtime;
	struct tm *lt;
	lt = localtime(&modTime);
	char date[12] = {0};
	strftime(date, 12, "%s", lt);
	//printf("date-%s, sizeof date-%d\n", date, sizeof(date));
	write(fd, date, 10); //write 10 digit date
	write(fd, fill, 1); //add space
	write(fd, fill, 1); //add space - now at size 12
	//write uid
	char uid[6] = {0};
	int uidI = (int)st.st_uid;
	sprintf(uid, "%d ", uidI);
	write(fd, uid, 6); //write 6 bytes for uid
	//write gid
	char gid[6] = {0};
	int gidI = (int)st.st_gid;
	sprintf(gid, "%d ", gidI);
	write(fd, gid, 6); //write 6 bytes for gid
	//write mode
	char mode[8] = {0};
	int modeI = (int)st.st_mode;
	sprintf(mode, "%o  ", modeI); //o for octal
	write(fd, mode, 8); //write 8 bytes for mode
	//write size
	char size[10] = {0};
	int sizeI = (int)st.st_size;
	sprintf(size, "%d", sizeI);
	write(fd, size, (int)strlen(size)); //write bytes for length of number (how many digits)
	int ten = (int)strlen(size);
	while(ten < 10){					//fill rest of 10 bytes with whitespace
		write(fd, fill, 1);
		ten++;
	}
	//write trailer string
	write(fd, ARFMAG, 2);
	//close file
	close(fd);
}

void fillBody(char *mem, char *arch){
	int afd = open(arch, O_RDWR, 0); //open arch file
	int mfd = open(mem, O_RDONLY, 0); //open member file
	int pos = 0;
	pos = lseek(afd, 0, 2); 		//seek to end of arch file
	struct stat st;
	if (stat(mem, &st) == -1){		//get stat data
		printf("ERROR: Stat failed.");
		exit(EXIT_FAILURE);
	}
	int size = 0;
	size = st.st_size; 				//size of mem file
	char temp[size + 1];		//temp buffer
	char nwl[1] = {'\n'};		//newline char
	read(mfd, temp, size);
	write(afd, temp, size);
	if(size % 2 != 0){			//adding newline char if odd
		write(afd, nwl, 1);
	}
	close(mfd);					//close files
	close(afd);
}

void xflag(int memNum, char **argv){
	char *archName = argv[2];	//set archName
	int fd = 0;					//init file descriptor
	int pos = 0;				//init position
	struct ar_hdr ah;			//init ar header struct
	char name[sizeof(ah.ar_name) + 1] = {0};	//name buffer
	char permB[sizeof(ah.ar_mode)+ 1] = {0};
	char perms[5] = {0};		//to hold only relevant perms
	int mode = 0;
	char sizeB[sizeof(ah.ar_size) + 1] = {0};	//size buffer
	int fsize = 0;								//holds size
	int mfd = 0;								//member file descriptor
	struct stat st;  						//use stat to get archive file size
	int asize = 0; 							//will hold archive file size
	
	if (access(archName, F_OK) == -1){ 	//check if archive exists
			printf("ERROR: Archive does not exist or cannot be accessed.\n");
			exit(EXIT_FAILURE);				//exit on failure if doesn't
		}
	//make sure not bad archive file
	else{
		char tms[9] = {0};						//to test magic string
		fd = open(archName, O_RDONLY, 0);	//open file
		read(fd, tms, 8);						//read magic string
		if(strcmp(tms, ARMAG) != 0){
			printf("ERROR: '%s' is non-standard archive.\n", archName);
			exit(EXIT_FAILURE);
		}
		close(fd);
	}
	//case for no members - need to extract all
	if(memNum == 0){
		fd = open(archName, O_RDONLY, 0); 		//open archive read only
		if(stat(archName, &st) == 0)
			asize = st.st_size; 				//size = archName file size
		pos = lseek(fd, 8, 0);				//seek to start of first header
		while(pos < asize){
			//find name
			int t = 0;
			while(t < sizeof(ah.ar_name)){
				read(fd, &name[t], 1);
				if(name[t] == '/'){
					name[t] = 0;		//delete terminating slash
					t++;
					pos = lseek(fd, 16 - t, 1);
					break;
				}
				t++;
			}
			//printf("FileName=%s&t=%d&pos=%d\n", name, t, pos);
			//find size
			pos = lseek(fd, 24, 1);		//seek past date12,uid6,gid6-total24
			read(fd, permB, sizeof(ah.ar_mode));	//read perms
			strncpy(perms, permB + 2, 4);			//keep only four digits ex0660
			mode = strtol(perms,NULL,8);			//convert perms to octal with leading 0
			read(fd, sizeB, sizeof(ah.ar_size));
			fsize = atoi(sizeB);
			//printf("Mode=%o|FileSize=%d|\n", mode, fsize);
			pos = lseek(fd, 2, 1); 		//seek past trailer string of 2. at beg of file
			mfd = creat(name, mode);	//create file with perms from mode in octal
			mfd = open(name, O_RDWR, 0);//open file with read write perms
			char transfer[fsize + 1];	//buffer to transfer into
			read(fd, transfer, fsize);	//read into transfer buffer
			//printf("Transfer=%s\n", transfer);
			int pass = write(mfd, transfer, fsize); //write to new file
			if(pass != fsize){
				printf("ERROR: Write failed for file: %s.\n", name); //ensure write passes
			}
			if(fsize % 2 != 0)			//if odd size file
				pos = lseek(fd, 1, 1);	//seek extra byte
			pos = lseek(fd, 0, 1);		//set pos properly for loop
			close(mfd);					//close member file
		}
		close(fd);
	}

	//case for extracting named members
	else if(memNum > 0){
		int c = 0; //count of members appended
		int i = 3; //iterator for argv
		for(c = 0; c < memNum; c++){
			char *mem = argv[i]; 	//set member
			extract(mem, archName);
			i++;
		}
		close(fd);
	}
}

void extract(char *mem, char *arch){
	int fd = 0;					//init file descriptor
	fd = open(arch, O_RDONLY, 0);
	int pos = 0;
	struct ar_hdr ah;			//init ar header struct
	struct stat st;  			//use stat to get archive file size
	int asize = 0;				//store archive size
	char name[sizeof(ah.ar_name) + 1] = {0}; //name buffer
	char permB[sizeof(ah.ar_mode)+ 1] = {0}; //mode/permission buffer
	char perms[5] = {0};		//to hold only relevant perms
	int mode = 0;				//hold octal mode
	char sizeB[sizeof(ah.ar_size) + 1] = {0}; //size buffer
	int fsize = 0;				//holds file size
	int mfd = 0;
	if(stat(arch, &st) == 0)
		asize = st.st_size; 	//size = archName file size
	pos = lseek(fd, 8, 0);	//seek to start of first header
	//printf("Mem=%s|\n", mem);
	while(pos < asize){
		//find name
		int t = 0;
		while(t < sizeof(ah.ar_name)){
			read(fd, &name[t], 1);
			if(name[t] == '/'){
				name[t] = 0;		//delete terminating slash
				t++;
				pos = lseek(fd, 16 - t, 1);
				break;
			}
			t++;
		}
		//printf("Name=%s|\n", name);
		if(strcmp(mem, name) == 0){
			pos = lseek(fd, 24, 1);				//seek past date12,uid6,gid6-total24
			read(fd, permB, sizeof(ah.ar_mode));//read perms
			strncpy(perms, permB + 2, 4);		//keep only four digits ex0660
			mode = strtol(perms,NULL,8);		//convert perms to octal with leading 0				
			read(fd, sizeB, sizeof(ah.ar_size));//store file size in buffer
			fsize = atoi(sizeB);				//atoi file size to fsize
			pos = lseek(fd, 2, 1);				//skip to start of file
			mfd = creat(name, mode);	//create file with perms from mode in octal
			mfd = open(name, O_RDWR, 0);//open file with read write perms
			char transfer[fsize + 1];	//buffer to transfer into
			read(fd, transfer, fsize);	//read into transfer buffer
			//printf("Transfer=%s\n", transfer);
			int pass = write(mfd, transfer, fsize); //write to new file
			if(pass != fsize){
				printf("ERROR: Write failed for file: %s.\n", name); //ensure write passes
			}
			if(fsize % 2 != 0)			//if odd size file
				pos = lseek(fd, 1, 1);	//seek extra byte
			pos = lseek(fd, 0, 1);		//set pos properly for loop
			close(mfd);					//close member file
			close(fd);
			return;
		}
		else{ //find next name
			pos = lseek(fd, 32, 1); //seek to file size
			read(fd, sizeB, sizeof(ah.ar_size));
			fsize = atoi(sizeB);
			pos = lseek(fd, fsize + 2, 1); //seek file size plus two for header trailer string
			if(fsize % 2 != 0)			//if odd size file
				pos = lseek(fd, 1, 1);	//seek extra byte
		}
	}
	close(fd);
	printf("File %s not found in archive.\n", mem);
	exit(EXIT_FAILURE);
}

void dflag(int memNum, char **argv){
	//exit if no members
	if(memNum == 0)
		exit(EXIT_SUCCESS);
	//exit if too many members
	if(memNum > 100){
		printf("ERROR: Only 100 files can be deleted per command.\n");
		exit(EXIT_FAILURE);
	}
	//set archive name
	char *archName = argv[2];
	//exit if archive doesn't exist
	if (access(archName, F_OK) == -1){
		printf("ERROR: Archive does not exist or cannot be accessed.\n");
		exit(EXIT_FAILURE);
	}
	//exit if bad archive
	else{
		char tms[9] = {0};						//to test magic string
		int fd = open(archName, O_RDONLY, 0);	//open file
		read(fd, tms, 8);						//read magic string
		if(strcmp(tms, ARMAG) != 0){
			printf("ERROR: '%s' is non-standard archive.\n", archName);
			exit(EXIT_FAILURE);
		}
		close(fd);
	}
	//variables
	//structs
	struct namesize {
		char name[17];	//store member name
		int fsize;		//store member size
		int start;		//store position of member in archive
	};
	struct namesize members[100]; 		//array of struct to hold name and filesize
	struct ar_hdr ah;					//init ar struct
	struct stat st;						//init stat struct
	stat(archName, &st);				//get archive stats
	int asize = st.st_size;				//set asize to archive size
	char sizeBuf[11] = {0};				//file size buffer
	int fd = open(archName, O_RDWR, 0);	//open archive file
	int pos = lseek(fd, 8, 0);			//seek past magic string
	int c = 0; 							//count of members appended
	//put names and sizes into struct
	while(pos < asize){
		int t = 0;
		while(t < sizeof(ah.ar_name)){
			read(fd, &members[c].name[t], 1);
			if(members[c].name[t] == '/'){
				members[c].name[t] = 0;		//delete terminating slash
				t++;
				pos = lseek(fd, 16 - t, 1);
				break;
			}
			t++;
		}
		pos = lseek(fd, 32, 1); 					//seek to file size
		read(fd, sizeBuf, 10); 						//read file size
		members[c].fsize = atoi(sizeBuf);
		if(members[c].fsize % 2 != 0)
			members[c].fsize += 1;					//add extra byte to size if odd
		if(c == 0){
			members[c].start = 8;
		}
		else{
			members[c].start = members[c-1].start + 60 + members[c-1].fsize;
		}
		pos = lseek(fd, members[c].fsize + 2, 1); 	//seek 2 for header trailer plus fsize
		//printf("MemName:%s|\n", members[c].name);
		//printf("FileSize:%d|\n", members[c].fsize);
		//printf("Offset:%d|\n", members[c].start);
		c++;
		//printf("Count:%d|\n", c); 
	}
	//put all cuts into array
	int cuts[c*2];
	int cc = 0;		//cut counter
	int i = 3;
	int k = 0;
	int j = 0;
	for(k = 0; k < memNum; k++){
		char *mem = argv[i];
		for(j = 0; j < c; j++){
			if(strcmp(mem, members[j].name) == 0){
				cuts[cc] = members[j].start;
				cc++;
				cuts[cc] = members[j].start + 60 + members[j].fsize;
				cc++;
				break;
			}
		}
		i++;
	}
	if(cc == 0){
		printf("Warning: no files found to delete.\n");
		exit(EXIT_SUCCESS);
	}
	//sort array
	qsort(cuts, cc, sizeof(int), compare);
	//print check sort
	/*for(k = 0; k < cc; k++){
		printf("%d\n", cuts[k]);
	}*/
	//make all deletes in one pass
	char temp[] = "qqxzqzQQp.a";					//name of temp file, random for no overwrite
	char buffer[asize + 1];							//create buffer size of archive
	int tfd = creat(temp, 0666);					//create new archive
	int numRead = 0;								//holds number of bytes read
	pos = lseek(fd, 0, 0);							//seek to old file start
	for(k = 0; k < cc; k++){						//loop for less than num of cuts
		numRead = read(fd, buffer, cuts[k] - pos);	//read until cut
		write(tfd, buffer, numRead);				//write to new file
		k++;										//move to next cut
		pos = lseek(fd, cuts[k]-cuts[k-1], 1);		//seek to end of cut
	}
	numRead = read(fd, buffer, asize - cuts[k-1]);	//read to eof
	write(tfd, buffer, numRead);					//write rest to temp
	close(fd);										//close old file
	unlink(archName);								//unlink old file
	rename(temp, archName);							//rename tempfile
	close(tfd);										//close new arch file
}

//compare function for qsort
//ref from cplusplus.com/reference/cstdlib/qsort
int compare(const void * a, const void * b){
	return (* (int*)a - *(int*)b);
}

void Aflag(int memNum, char **argv){
	//ref to "The C Programming Language" 2nd Ed. by Kernighan/Ritchie ch8
	//for help understanding dirent.h to read directories
	//set archive name
	char *archName = argv[2];
	int fd = 0;		//init file descriptor
	int del = 0;	//delete flag
	//create archive if doesn't exist
	if (access(archName, F_OK) == -1){
		if((fd = creat(archName, 0666)) == -1){
			printf("ERROR: Can't create %s\n", archName);
			exit(EXIT_FAILURE);
		}
		fd = open(archName, O_RDWR, 0); //open file
		write(fd, ARMAG, SARMAG);		//write ar magic string
		close(fd);						//close file 
	}
	//archive already exists - test if ok
	else{
		char tms[9] = {0};					//to test magic string
		fd = open(archName, O_RDONLY, 0);	//open file
		read(fd, tms, 8);					//read magic string
		if(strcmp(tms, ARMAG) != 0){
			printf("ERROR: '%s' is non-standard archive.\n", archName);
			exit(EXIT_FAILURE);
		}
		close(fd);
	}
	//warning for member args on command line
	if(memNum > 0){
		printf("Warning: members are ignored with this command.\n");
	}
	char cwd[512];												//512 set as max path size
	if(getcwd(cwd, sizeof(cwd)) == NULL){						//get current dir
		printf("ERROR: Could not get current directory.\n");
		exit(EXIT_FAILURE);
	}
	struct stat st;												//declare stat struct
	DIR *dfd;													//directory file descriptor
	struct dirent *dp;											//declare dirent struct - NOTE: alternately declared as Dirent with name not d_name
	dfd = opendir(cwd);
	if(dfd == NULL){
		printf("ERROR: Cannot open directory '%s'\n", cwd);
		exit(EXIT_FAILURE);
	}
	while((dp = readdir(dfd)) != NULL){
		if(strcmp(dp->d_name, archName) == 0){					//do not copy archive into self
			continue;
		}
		if(strcmp(dp->d_name, "myar") == 0){					//do not copy file into archives
			continue;
		}
		if(stat(dp->d_name, &st) != 0){
			printf("ERROR: stats on file: %s could not be gathered.\n", dp->d_name);
			exit(EXIT_FAILURE);
		}
		if(!S_ISREG(st.st_mode)){	//check if regular file
			continue;				//continue if not regular
		}
		//here file is regular and is not the archive
		//printf("%s|\n", dp->d_name);
		del = deleteOld(dp->d_name, archName);		//returns 0 if need to add
		if(del == 1){								//returns 1 if no add
			printf("File: '%s' has identical copy in archive. File not added.\n", dp->d_name);
			continue;								//continue w/o adding
		}
		fillHeader(dp->d_name, archName);
		fillBody(dp->d_name, archName);
		if(del == 2){
			printf("Old version of '%s' removed from archive. New version added.\n", dp->d_name);
		}
	}
	closedir(dfd);
}

int deleteOld(char *mem, char *arch){
	int t = 0, exists = 0, delFlag = 0;
	struct header{		//header info struct
		char name[17];	//store name
		int mtime;		//date from ar
		int uid;		//uid
		int gid;		//gid
		int mode;		//permissions
		int fsize;		//file size
	};
	struct header hi;					//init hi struct
	struct ar_hdr ah;					//init ar struct
	struct stat st;						//init stat struct
	stat(arch, &st);					//get archive stats
	int asize = st.st_size;				//set asize to archive size
	char buffer[13] = {0};				//to hold ar values
	int fd = open(arch, O_RDWR, 0);		//open archive
	int pos = lseek(fd, 8, 0);			//seek past magic string
	while(pos < asize){
		t = 0;
		while(t < sizeof(ah.ar_name)){
			read(fd, &hi.name[t], 1);
			if(hi.name[t] == '/'){
				hi.name[t] = 0;		//delete terminating slash
				t++;
				pos = lseek(fd, 16 - t, 1);
				break;
			}
			t++;
		}
		if(strcmp(hi.name, mem) == 0){
			exists = 1;
			break;
		}
		pos = lseek(fd, 32, 1); 					//seek to file size
		read(fd, buffer, 10); 						//read file size
		hi.fsize = atoi(buffer);
		if(hi.fsize % 2 != 0)
			hi.fsize += 1;							//add extra byte to size if odd
		pos = lseek(fd, hi.fsize + 2, 1); 			//seek 2 for header trailer plus fsize
	}
	if(exists == 0){								//if exists == 0, file is not in archive
		return 0;									//file needs to be added
	}
	//file is in archive, needs to be compared to
	read(fd, buffer, 12);					//read date
	hi.mtime = atoi(buffer);				//store date in mtime
	memset(&buffer[0], 0, sizeof(buffer)); 	//clear buffer
	read(fd, buffer, 6);					//read uid
	hi.uid = atoi(buffer);					//store uid
	read(fd, buffer, 6);					//read gid
	hi.gid = atoi(buffer);					//store gid
	read(fd, buffer, 8);					//read mode
	hi.mode = strtol(buffer, NULL, 8);		//store mode as decimal conversion like stat
	read(fd, buffer, 10);					//read size
	hi.fsize = atoi(buffer);				//store fszie
	pos = lseek(fd, 2, 1);					//seek past trailer string
	//printf("Name: %s Date: %d UID: %d\nGID: %d Mode: %d Size: %d\n", hi.name, hi.mtime, hi.uid, hi.gid, hi.mode, hi.fsize);
	
	stat(mem, &st);							//get mem stats
	//printf("Name: %s Date: %d UID: %d\nGID: %d Mode: %d Size: %d\n", mem, st.st_mtime, st.st_uid, st.st_gid, st.st_mode, st.st_size);
	if(hi.mtime != st.st_mtime){
		delFlag++;
	}
	else if(hi.uid != st.st_uid){
		delFlag++;
	}
	else if(hi.gid != st.st_gid){
		delFlag++;
	}
	else if(hi.mode != st.st_mode){
		delFlag++;
	}
	else if(hi.fsize != st.st_size){
		delFlag++;
	}
	else{
		return 1;
	}
	//delete old file version
	int cut1 = pos - 60;					//pos at end of header. set cut1 to beginning of header (size 60)
	if(hi.fsize % 2 != 0)
		hi.fsize += 1;						//add extra byte to size if odd
	pos = lseek(fd, hi.fsize, 1); 			//seek to end of fsize
	int cut2 = pos;							//set cut2 to end of file
	//make delete in one pass
	char temp[] = "QQzzXXubfr5f.a";					//name of temp file, random for no overwrite
	char buff[asize + 1];							//create buffer size of archive
	int tfd = creat(temp, 0666);					//create new archive
	int numRead = 0;								//holds number of bytes read
	pos = lseek(fd, 0, 0);							//seek to old file start
	numRead = read(fd, buff, cut1 - pos);			//read until cut
	write(tfd, buff, numRead);						//write to new file
	pos = lseek(fd, cut1-cut2, 1);					//seek to end of cut
	numRead = read(fd, buff, asize - cut2);			//read to eof
	write(tfd, buff, numRead);						//write rest to temp
	close(fd);										//close old file
	unlink(arch);									//unlink old file
	rename(temp, arch);								//rename tempfile
	close(tfd);										//close new arch file
	return 2;
}

void wflag(int memNum, char **argv){
	//set archive name
	char *archName = argv[2];
	//exit if archive doesn't exist
	if (access(archName, F_OK) == -1){
		printf("ERROR: Archive does not exist or cannot be accessed.\n");
		exit(EXIT_FAILURE);
	}
	//exit if bad archive
	else{
		char tms[9] = {0};						//to test magic string
		int fd = open(archName, O_RDONLY, 0);	//open file
		read(fd, tms, 8);						//read magic string
		if(strcmp(tms, ARMAG) != 0){
			printf("ERROR: '%s' is non-standard archive.\n", archName);
			exit(EXIT_FAILURE);
		}
		close(fd);
	}
	//check usage
	if(memNum != 1){
		printf("ERROR: %s %s <archive> <seconds> is the usage.\n", argv[0], argv[1]);
		exit(EXIT_FAILURE);
	}
	//check valid timeout
	int timeout = atoi(argv[3]);
	if(timeout < 1 || timeout > 1000){
		printf("ERROR: Timeout of %d is out of range (0-1000 seconds).\n", timeout);
		printf("Usage: %s %s <archive> <seconds> \n", argv[0], argv[1]);
		exit(EXIT_FAILURE);
	}
	//set up working in directory
	char cwd[512];												//512 set as max path size
	if(getcwd(cwd, sizeof(cwd)) == NULL){						//get current dir
		printf("ERROR: Could not get current directory.\n");
		exit(EXIT_FAILURE);
	}
	DIR *dfd;													//directory file descriptor
	struct dirent *dp;											//declare dirent struct - NOTE: alternately declared as Dirent with name not d_name
	//set up structs
	struct stat st;												//declare stat struct
	struct compare{
		char name[17];											//hold name
		int mtime;												//hold mod time
	};
	struct compare pre[100];									//only 100 regular files can be in directory
	struct compare post[100];									//only 100 regular files can be in directory
	int e = 0;													//count of files in pre struct
	int t = 0;													//count of files in post struct
	//get precondition of regular files in directory
	dfd = opendir(cwd);
	if(dfd == NULL){
		printf("ERROR: Cannot open directory '%s'\n", cwd);
		exit(EXIT_FAILURE);
	}
	while((dp = readdir(dfd)) != NULL){
		if(strcmp(dp->d_name, archName) == 0){					//do not copy archive into self
			continue;
		}
		if(strcmp(dp->d_name, "myar") == 0){					//do not copy file into archives
			continue;
		}
		if(stat(dp->d_name, &st) != 0){
			printf("ERROR: stats on file: %s could not be gathered.\n", dp->d_name);
			exit(EXIT_FAILURE);
		}
		if(!S_ISREG(st.st_mode)){	//check if regular file
			continue;				//continue if not regular
		}
		strcpy(pre[e].name, dp->d_name);
		pre[e].mtime = st.st_mtime;
		e++;
		if(e > 100){
			printf("ERROR: Too many files in this directory for this command. Need less than 100.\n");
			exit(EXIT_FAILURE);
		}
	}
	closedir(dfd);
	//wait timeout	
	int timeElapsed = 0;
	printf("Waiting %d seconds...\n", timeout);
	while(timeElapsed < timeout){
		sleep(1);
		timeElapsed++;
		if(timeElapsed % 5 == 0){
			printf("Waited %d of %d seconds.\n", timeElapsed, timeout);
		}
	}
	//allow SIGINT?
	//get post-condition of files in archive
	dfd = opendir(cwd);
	if(dfd == NULL){
		printf("ERROR: Cannot open directory '%s'\n", cwd);
		exit(EXIT_FAILURE);
	}
	while((dp = readdir(dfd)) != NULL){
		if(strcmp(dp->d_name, archName) == 0){					//do not copy archive into self
			continue;
		}
		if(strcmp(dp->d_name, "myar") == 0){					//do not copy file into archives
			continue;
		}
		if(stat(dp->d_name, &st) != 0){
			printf("ERROR: stats on file: %s could not be gathered.\n", dp->d_name);
			exit(EXIT_FAILURE);
		}
		if(!S_ISREG(st.st_mode)){	//check if regular file
			continue;				//continue if not regular
		}
		strcpy(post[t].name, dp->d_name);
		post[t].mtime = st.st_mtime;
		t++;
		if(t > 100){
			printf("ERROR: Too many files in this directory for this command. Need less than 100.\n");
			exit(EXIT_FAILURE);
		}
	}
	closedir(dfd);
	//compare, add new if different
	int i = 0;
	int j = 0;
	for(i = 0; i < t; i++){
		while(j < e){
			if(strcmp(post[i].name, pre[j].name) == 0 && post[i].mtime != pre[j].mtime){
				fillHeader(post[i].name, archName);
				fillBody(post[i].name, archName);
				printf("File %s changed. It has been added to archive.\n", post[i].name);
				break;
			}
			j++;
		}
		j = 0;
	}
}