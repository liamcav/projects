/*Liam Caveney 
  CSC 360 - assignment 3 part 3 
  Oct. 2016
  
  Program copies file from current linux dir to root dir of FAT12 disk image and updates information
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int sector_size = 512;
int fileSizei;

unsigned char fileName[8];
unsigned char ext[3];
unsigned char att[1];
unsigned char crtime[2];
unsigned char crdate[2];
unsigned char ladate[2];
unsigned char lwtime[2];
unsigned char lwdate[2];
unsigned char flc[2];
unsigned char fileSize[4];

unsigned char disk[1474560];


void getInfo(FILE* f, char* s){
	//name & ext
	int i;
	int j=0;
	int k=0;
	int len=strlen(s)-4;
	for(i=0; i<12; i++){
		if(i<len){
			fileName[i]=s[j];
			j++;
		} else if(i<8){
			fileName[i]=' ';
		} else if(i==8){
			j++;		
		} else{	
			ext[k]=s[j];
			j++;
			k++;
		}	
	}
	
	//att
	att[0]=0;
	
	//dates and times/
	time_t rawtime;
	struct tm *info;
	time(&rawtime);
	info=localtime(&rawtime);
	int year = info->tm_year;
	year=year+1900-1980;	
	int month=(info->tm_mon)+1;	
	int day=info->tm_mday;
	lwdate[1]=ladate[1]=crdate[1]=(year<<1)+(month>>3);	
	lwdate[0]=ladate[0]=crdate[0]=((month&0x07)<<5)+day;
	
	int hour=info->tm_hour;
	int min=info->tm_min;
	int sec=info->tm_sec;
	lwtime[1]=crtime[1]=(hour<<3)+(min>>3);
	lwtime[0]=crtime[0]=((min&0x07)<<5)+sec;
	
	
	//fileSize
	fseek(f, 0, SEEK_END);
	fileSizei=ftell(f);
	fileSize[3]=fileSizei>>24;
	fileSize[2]=(fileSizei>>16);
	fileSize[1]=(fileSizei>>8);
	fileSize[0]=fileSizei;	
	fseek(f, 0, SEEK_SET);	
}

void createDirEntry(int i){
	int j;
	for(j=0; j<8; j++){
		disk[i]=toupper(fileName[j]);
		i++;
	}
	for(j=0; j<3; j++){
		disk[i]=toupper(ext[j]);
		i++;
	}
	disk[i]=att[0];
	i+=3;
	for(j=0; j<2; j++){
		disk[i]=crtime[j];
		i++;
	}
	for(j=0; j<2; j++){
		disk[i]=crdate[j];		
		i++;
	}
	for(j=0; j<2; j++){
		disk[i]=ladate[j];
		i++;
	}			
	i+=2;
	for(j=0; j<2; j++){
		disk[i]=lwtime[j];
		i++;
	}
	for(j=0; j<2; j++){
		disk[i]=lwdate[j];
		i++;
	}
	i+=2;//skip first logical cluster
	for(j=0; j<4; j++){
		disk[i]=fileSize[j];
		i++;
	}	
}

int calcFreeSpace(FILE* fp){
	int i;
	//total size
	unsigned char totalSectorCount[2];
	fseek(fp, 19, SEEK_SET);
	fread(totalSectorCount, 1, 2, fp);
	int secCount = totalSectorCount[1]<<8;
	secCount+=totalSectorCount[0];
	int totalSize=secCount*sector_size;
		
	int takenSecCount=0;
	for(i=0; i<2364; i++){ 
		unsigned char eight[1];
		unsigned char four[1];
		if(i%2==0){
			fseek(fp, 512+(3*i)/2, SEEK_SET);
			fread(eight, 1, 1, fp);
			fread(four, 1, 1, fp);
			four[0]=four[0]&0x0f;
			if(four[0]!=0 || eight[0]!=0){					
				takenSecCount++;
			}			
		} else{
			fseek(fp, 512+(3*i)/2, SEEK_SET);
			fread(four, 1, 1, fp);
			four[0]=four[0]&0xf0;
			fread(eight, 1, 1, fp);	
			if(eight[0]!=0 || four[0]!=0){
				takenSecCount++;
			}			
		}
	}	
	
	return totalSize-takenSecCount*sector_size;	
}


int main(int argc, char* argv[]){
	if(argc<3){
		printf("Error: not enough arguments\n");
		return 0;
	}
	FILE* fpf=fopen(argv[2], "r");
	if(fpf==NULL){
		printf("File not found.\n");
		return 0;
	}	
	getInfo(fpf, argv[2]);
	unsigned char fileArr[fileSizei];
	fread(fileArr, 1, fileSizei, fpf);
	
	FILE* fpd=fopen(argv[1], "r"); 
	fread(disk, 1, 1474560, fpd);
	int freeSpace=calcFreeSpace(fpd);
	if(freeSpace<fileSizei){
		printf("Not enough free space in the disk image.\n");
		return 0;
	}
	int i=0x2600;
	while(disk[i]!=0){
		i+=32;
	}
	createDirEntry(i);
		
	//find first FAT empty and update flc
	int n=2;
	int flc;
	int prev=0;
	int first=1;
	int fileByteCount=0;
	
	while (fileSizei>0){
	
		//go through FAT table, finding empty sector, mark curr
		while(1){ 
			if(n%2==0){
				if(disk[512+(3*n)/2]==0){				
					if((disk[512+1+(3*n)/2]&0x0F)==0){
						if (first==1){							
							flc=n;					
						}
						break;			
					}	
				}	
			} else{
				if(disk[512+1+(3*n)/2]==0){
					if((disk[512+(3*n)/2]&0xF0)==0){
						if(first==1){	
							flc=n;														
						}
						break;	
					}
				}			
			}
			n++;
		}
		
		//marks as last entry, will be overwritten if it isnt
		if(n%2==0){
			disk[512+(3*n)/2]=0xff;  //8
			disk[512+1+(3*n)/2]=0x0f;  //4
		} else{
			disk[512+1+(3*n)/2]=0xff;  //8 
			disk[512+(3*n)/2]|=0xf0;   //4			
		}
		
		//if not first, enter curr into prev
		if (first==1){
			first=0;
		} else{
			if(prev%2==0){
				disk[512+(3*prev)/2]=n&0x0ff;  //8
				disk[512+1+(3*prev/2)]=(disk[512+1+(3*prev/2)]&0xf0) | n>>8;  //4
			} else{
				disk[512+1+(3*prev)/2]=n>>4;  //8
				disk[512+(3*prev)/2]= (disk[512+(3*prev)/2]&0x0f) | ((n&0x00f)<<4);  //4
			}					
		}
				
		
		
		
		//copy data into empty sector
		int j;
		for(j=0; j<512 && j<fileSizei; j++){			
			disk[512*(n+31)+j]=fileArr[fileByteCount];  
			fileByteCount++;
		}	
		
		prev=n;
		n++;
		fileSizei-=512;
	}
	
	disk[i+26]=flc&0x00FF;
	disk[i+27]=(flc&0xFF00)>>8;
	
	fclose(fpd);
	FILE* fpd2=fopen(argv[1], "w");
	fwrite(disk, 1, 1474560, fpd2);
	
	return 0;
}		  
	
				 
	

	
	
