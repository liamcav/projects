/*Liam Caveney
  CSC 360 - assignment 3 part 2 
  Oct. 2016
  
  Program lists contents of FAT12 root directory
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
	
	//number of files(excluding subdir)
	unsigned char firstByte[1];                   
	fseek(fp, sector_size*19, SEEK_SET);
	fread(firstByte, 1, 1, fp);
	while(firstByte[0]!=0){
		if(firstByte[0]!=0xE5){
		
			unsigned char name[9];
			name[8]='\0';
			unsigned char ext[4];
			ext[3]='\0';
			unsigned char attr[1];
			unsigned char size[4];
			unsigned char date[2];
			unsigned char time[2];
			
			fseek(fp, -1, SEEK_CUR);
			fread(name, 1, 8, fp);
			fread(ext, 1, 3, fp);
			fread(attr, 1, 1, fp);
			fseek(fp, 2, SEEK_CUR);
			fread(time, 1, 2, fp);
			fread(date, 1, 2, fp);
			fseek(fp, 10, SEEK_CUR);
			fread(size, 1, 4, fp);
			
			if(attr[0]!=0x0f && attr[0]!=0x08){
				if(attr[0]==0x10){
					printf("D ");
				} else{
					printf("F ");
				}
				int sizei=0;
				sizei=size[3]<<(3*8);
				sizei+=size[2]<<(2*8);
				sizei+=size[1]<<(1*8);
				sizei+=size[0];
				printf("%10i ", sizei);
				
				//formatting of name
				unsigned char namef[9];
				namef[8]='\0';
				int i;
				int numSpace=0;
				for(i=0; i<8; i++){
					if(name[i]==' '){
						numSpace++;
					}
				}
				for(i=0; i<numSpace; i++){
					namef[i]=' ';
				}
				for(i=0; i<8-numSpace; i++){
					namef[i+numSpace]=name[i];
				}				
				printf("%17s.%s ", namef, ext);

				int year, month, day, hour, min;	
				year=date[1]>>1;
				year+=1980;
				month=(date[1]&0x01)<<3;
				month+=date[0]>>5;
				day=(date[0]&0x1F);
				
				hour=time[1]>>3;
				min=(time[1]&07)<<3;
				min+=(time[0]&0xE0)>>5;
				
						
				printf("%i-%i-%i %02i:%02i\n", year, month, day, hour, min);	
			}
		}
		fread(firstByte, 1, 1, fp);
	}			

	return 0;
}
