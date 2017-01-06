/*Liam Caveney 
  CSC 360 - assignment 3 part 1 
  Oct. 2016
  
  Program provides information on FAT12 disk image
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int sector_size = 512;

int main(int argc, char* argv[]){
	if(argc<2){
		printf("Error: need file as argument\n");
		return 0;
	}
	FILE* fp=fopen(argv[1], "r");
	
	unsigned char name[8];  
	fseek(fp, 3, SEEK_SET);
	fread(name, 1, 8, fp);
	char nameString[9];
	int i;	
	for(i=0; i<8; i++){
		nameString[i]=name[i];
	}
	nameString[i]='\0';
	printf("OS Name: %s\n", nameString);
	
	//number of files(excluding subdir) and label
	unsigned char label[9];
	label[8]='\0';
	unsigned char firstByte[1];                   
	int fileCount=0;
	fseek(fp, sector_size*19, SEEK_SET);
	fread(firstByte, 1, 1, fp);
	while(firstByte[0]!=0){
		if(firstByte[0]!=0xE5){
			unsigned char attr[1];
			fseek(fp, 10, SEEK_CUR);
			fread(attr, 1, 1, fp);
			if(attr[0]==0x08){   //volume label found
				fseek(fp, -12, SEEK_CUR);
				fread(label, 1, 8, fp);
				fseek(fp, 4, SEEK_CUR);
			}
			if(attr[0]!=0x0f && (attr[0]&0x10)==0 && attr[0]!=0x08){
				fileCount++;
			}
		}	
		fseek(fp, 20, SEEK_CUR);	
		fread(firstByte, 1, 1, fp);
	}		
	
	printf("Label of the disk: %s\n",label);
	
	//total size
	unsigned char totalSectorCount[2];
	fseek(fp, 19, SEEK_SET);
	fread(totalSectorCount, 1, 2, fp);
	int secCount = totalSectorCount[1]<<8;
	secCount+=totalSectorCount[0];
	int totalSize=secCount*sector_size;
	printf("Total size of the disk: %i bytes\n",totalSize);	
	
	
	int takenSecCount=0;
	for(i=0; i<2364; i++){ //2847??
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
	
	printf("Free size of the disk: %i bytes\n", totalSize-takenSecCount*sector_size);

	printf("\n==============\n");
	
	printf("The number of files in the root directory (not including subdirectories): %i\n", fileCount);

	printf("\n==============\n");

	//number of FAT copies
	unsigned char numFATs[1];	
	fseek(fp, 16, SEEK_SET);
	fread(numFATs, 1, 1, fp);
	printf("Number of FAT copies: %i\n", numFATs[0]);
	
	//sectors per FAT 
	unsigned char secPerFAT[2];   //how is this num stored?? endian?
	fseek(fp, 22, SEEK_SET);
	fread(secPerFAT, 1, 2, fp);
	int spf=(secPerFAT[1]<<4)+secPerFAT[0];  
	printf("Sectors per FAT: %i\n", spf);

	return 0;
}


