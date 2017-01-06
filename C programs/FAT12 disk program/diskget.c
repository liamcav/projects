/*Liam Caveney 
  CSC 360 - assignment 3 part 3 
  Oct. 2016
  
  Program copies file from FAT12 disk image root dir to current linux dir 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int sector_size = 512;

void copyFile(FILE* fp, char* name){
	FILE* fp2 = fopen(name, "a");

	unsigned char FATnumRead[2];
	fread(FATnumRead, 1, 2, fp);
	unsigned char fileSizeRead[4];
	fread(fileSizeRead, 1, 4, fp);
	int fileSize=fileSizeRead[0]+(fileSizeRead[1]<<8)+(fileSizeRead[2]<<16)+(fileSizeRead[3]<<24);
	int FATnum=FATnumRead[0]+(FATnumRead[1]<<8);
	int physnum;
	
	while(FATnum<0xFF0){
		//if(FATnum!=0){
			physnum=512*(31+FATnum);
			fseek(fp, physnum, SEEK_SET);
			int i=0;
			while(i<512 && i<fileSize){
				unsigned char temp[1];
				fread(temp, 1, 1, fp);
				fputc(temp[0], fp2);
				i++;
			}
			fileSize-=512;
		
			unsigned char eight[1];
			unsigned char four[1];
			if(FATnum%2==0){
				fseek(fp, 512+(3*FATnum)/2, SEEK_SET);
				fread(eight, 1, 1, fp);
				fread(four, 1, 1, fp);
				four[0]=four[0]&0x0f;
				FATnum=eight[0]+(four[0]<<8);
						
			} else{
				fseek(fp, 512+(3*FATnum)/2, SEEK_SET);
				fread(four, 1, 1, fp);

				four[0]=(four[0]&0xf0)>>4;
				fread(eight, 1, 1, fp);
				FATnum=four[0]+(eight[0]<<4);
			}
		//}
	}	
}



int main(int argc, char* argv[]){
	if(argc<3){
		printf("Error: not enough arguments\n");
		return 0;
	}
	FILE* fp=fopen(argv[1], "r"); 
	char fileName[9];
	fileName[8]='\0';
	char fileExt[4];
	fileExt[3]='\0';
	int i;
	int j=0;
	int k=0;
	int len=strlen(argv[2])-4;
	for(i=0; i<12; i++){
		if(i<len){
			fileName[i]=argv[2][j];
			j++;
		} else if(i<8){
			fileName[i]=' ';
		} else if(i==8){
			j++;		
		} else{	
			fileExt[k]=argv[2][j];
			j++;
			k++;
		}	
	}
				
	int found=0;
	unsigned char firstByte[1];                   
	fseek(fp, sector_size*19, SEEK_SET);
	fread(firstByte, 1, 1, fp);
	while(firstByte[0]!=0){
		if(firstByte[0]!=0xE5){
		
			char name[9];
			name[8]='\0';
			char ext[4];
			ext[3]='\0';
			fseek(fp, -1, SEEK_CUR);
			fread(name, 1, 8, fp);
			fread(ext, 1, 3, fp);	
			if(strcasecmp(name, fileName)==0 && strcasecmp(ext, fileExt)==0){
				found=1;
				fseek(fp, 15, SEEK_CUR);
				copyFile(fp, argv[2]);
				break;
			} 
		}
		fseek(fp, 21, SEEK_CUR);
		fread(firstByte, 1, 1, fp);	
	}	
	if(found==0){
		printf("File not found.\n");
	}
		
	return 0;
}	
