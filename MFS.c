/*Liam Caveney 
  CSC 360 - assignment 2

  Multi-Flow Task Scheduler that simulates prioritised flow arrivals and transmissions 		
  
  Oct. 2016
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

struct flow{
	int id, priority; 
	float arr_time, tran_time; 
};

int id_running;

int list_size;
struct flow list[1000];

int PQ_size;
struct flow PQ[1000];
int PQtop=0;

pthread_t thrList[1000];
pthread_mutex_t mutex;
pthread_cond_t convar; 
clock_t clock_start;

int processed=0;
int MFSbusy=0;

struct timeval initial;

//printQueue() method used for testing
/*void printQueue(){
	int i;
	for(i=PQtop; i<PQ_size; i++){
		printf("%i, ", PQ[i].id);
	}
	printf("\n");
	return;
}*/

void list_insert(int i, int a, int t, int p){

	struct flow* item = (struct flow*)malloc(sizeof(struct flow));
	item->id=i;
	item->arr_time=a;
	item->tran_time=t;
	item->priority=p;
	list[list_size]=*item; 
	list_size++;	
}

//returns 1 if new is higher priority, 0 if old is higher priority
int compare_priority(struct flow new, struct flow old){
	if(new.priority < old.priority){
		return 1;
	}else if(&new.priority < &old.priority){
		return 0;
	}else if (&new.arr_time < &old.arr_time){
		return 1;
	}else if(&new.arr_time > &old.arr_time){
		return 0;
	}else if(&new.tran_time < &old.arr_time){
		return 1;
	}
	return 0;
}

//insterts flow into PQ
int PQ_insert(struct flow *curr){
	if(PQ_size==0){
		PQ[0]=*curr;
		PQ_size++;
		return 0;
	}	
	int i;
	for(i=0; i<PQ_size; i++){
		int place=compare_priority(*curr, PQ[i]);
		if (place==1){
			int j;			
			for(j=PQ_size-1; j>=i; j--){
				PQ[j+1]=PQ[j];
			}	
			break;
		}	
	} 
	PQ[i]=*curr;
	PQ_size++;
	return i;
}

//waits until threads turn to run 
void requestPipe(struct flow *item){
	pthread_mutex_lock(&mutex);

	if(MFSbusy==0 && PQ_size==0){
		MFSbusy=1;
		id_running=item->id;
		pthread_mutex_unlock(&mutex);
		return;
	}
	int place=PQ_insert(item);	
	
	while(item->id!=PQ[0].id || MFSbusy==1){
		int l=0;
		while(l<PQ_size){
			if(PQ[l].id==item->id){
				break;
			}
			l++;
		}
		if(l==0){
			printf("Flow %i waits for the finish of flow %2i.\n", item->id, id_running);
		} else{
			printf("Flow %i waits for the finish of flow %2i.\n", item->id, PQ[l-1].id);
		}
		pthread_cond_wait(&convar, &mutex);
	}
	int i;		
	for(i=1; i<PQ_size; i++){
		PQ[i-1]=PQ[i];
	}	
	PQ_size--;
	MFSbusy=1;
	pthread_mutex_unlock(&mutex);
}


void releasePipe(){
	MFSbusy=0;
}

//thread function
void *thrFunction(void *flowItem){
	struct flow *item=(struct flow *)flowItem;
	struct timeval start, finish;

	if(usleep((item->arr_time)*100000)==-1){
		printf("Error: unable to sleep");
	}
	printf("Flow %i arrives: arrival time (%.1f), transmission time (%.1f), priority (%i).\n", item->id, item->arr_time/10, item->tran_time/10, item->priority);

	requestPipe(item);
	id_running=item->id;
	if(gettimeofday(&start, NULL)!=0){
		printf("Error: unable to get time");
		return 0;
	}
	printf("Flow %i starts its transmission at time %.1f.\n", item->id, ((double)(((start.tv_sec - initial.tv_sec)*1000000L+start.tv_usec) - initial.tv_usec)/1000000));
	if(usleep((item->tran_time)*100000)){
		printf("Error: unable to sleep");
	}

	releasePipe(item);
	
	if(gettimeofday(&finish, NULL)!=0){
		printf("Error: unable to get time");
		return 0;
	}
	printf("Flow %i finishes its transmission at time %.1f.\n", item->id, ((double)(((finish.tv_sec - initial.tv_sec)*1000000L+finish.tv_usec) - initial.tv_usec)/1000000));
	pthread_cond_broadcast(&convar);// use broadcast
	pthread_exit(NULL);
}


int main(int argc, char **argv){
	gettimeofday(&initial, NULL);

	if(argc<2){	
		printf("Error: need filename as argument\n");
		return 0;
	}
	FILE *fp;
	int num;
	char buff[100];
	list_size=0;
	PQ_size=0;

	fp=fopen(argv[1], "r");
	fscanf(fp, "%d", &num);
	if (num==0){ 
		return 0;
	}
	fgets(buff, 2, fp); //move fp to next line
	
	int i;
	for(i=0; i<num; i++){
		int id, priority;
		float arr_time, tran_time;
		char* ptr=buff;
		fgets(buff, 100, fp);
		int j=0;		
		while(ptr[j]!=58){
			j++;
		}
		char str[j+1];
		strncpy(str, ptr, j);
		str[j]='\0';
		id=atoi(str);
		ptr=ptr+j+1;
		j=0;

		while(ptr[j]!=44){
			j++;
		}
		char str2[j+1];
		strncpy(str2, ptr, j);
		str2[j]='\0';
		arr_time=atof(str2);
		ptr=ptr+j+1;
		j=0;

		while(ptr[j]!=44){
			j++;
		}
		char str3[j+1];
		strncpy(str3, ptr, j);
		str3[j]='\0';
		tran_time=atof(str3);
		ptr=ptr+j+1;
		j=0;
		
		while(ptr[j]!='\0'){
			j++;
		}
		char str4[j+1];
		strncpy(str4, ptr, j);
		str4[j]='\0';
		priority=atoi(str4);
		ptr=ptr+j+1;
		j=0;
		
		list_insert(id, arr_time, tran_time, priority);
		
	}
	fclose(fp); 

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&convar, NULL);
	
	for(i=0; i<list_size; i++){  
		if(pthread_create(&thrList[i], NULL, thrFunction, (void*)&list[i])!=0){
			printf("Error: unable to create all threads");
			return 0;
		}
	}

	//waitfor all threads to join
	for(i=0; i<num; i++){
		if(pthread_join(thrList[i], NULL)!=0){
			printf("Error: not all threads able to rejoin");
			return 0;
		}
	}
	//destroy mutex and cond var
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&convar);
	pthread_exit(NULL);  

	return 0;
}


