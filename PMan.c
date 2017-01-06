//Liam Caveney CSC360
//PMan.c acts as shell that can start stop and kill background processes and give information on currently running processes 

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>

//struct for process
struct node{
	pid_t pid;
	char* name;
	//int isStop;
	struct node* next;
};

struct node* head =NULL;
struct node* current=NULL;



void update_bg_process(){
	while(1){
//printf("updating");
		int status;
		int id=waitpid(-1, &status, WNOHANG);
		if (id==0)
			break;
		if (id>0){
			printf("Process %i has terminated.\n", id);
			//update linked list
			if(head->pid==id){
				free(head);
				head=head->next;
			}
			else{				
				current=head->next;
				struct node* prev=current;
				while(current->pid != id && current->next!=NULL){
					current=current->next;
				}
				if(current->pid==id){
					prev->next=current;					
					free(current);
					
				}
			}
		}
		if(id<0)
			break;
	}
	return;
}


//bg program - runs program in bg  Use linked list to record bg processes
void bgbegin(char* s){
	s=s+3;	
	
	char* ar[100];
	char* token;	
	int i;
	token = strtok(s, " ");

	i=0;
	while( token != NULL ) {
	    ar[i] = malloc(strlen(token) + 1);
	    strncpy(ar[i], token, strlen(token));
	    i++;
	    token = strtok(NULL, " ");
	    } 
	ar[i] = NULL; 
		
	char* cmd=ar[0];
	pid_t childpid=fork();
	if (childpid==0){
		execvp(cmd, ar);
			//shouldnt execute
			printf("input: command not found\n");
			exit(-1);
		
	}else{
		sleep(1);//wait(1);
		struct node* link =(struct node*)malloc(sizeof(struct node));
		link->pid=childpid;
		link->name=s;
		link->next=head;
		head=link;
		update_bg_process();
		
	}			
}

//bglist - displays all programs running in bg
void bglist(){
	printf("\n");

	int count=0;
	current=head;
	while(current!=NULL){
		int curpid=current->pid;
		char proc_list[100];		
		char path[100];
		sprintf(proc_list, "/proc/%i/exe", curpid);
		readlink(proc_list, path, sizeof(path));
		int i=0;		
		while(path[i]=='/'||isascii(path[i]))
			i++;
		path[i]='\0';
		printf("%i: %s\n", current->pid, path);//'/proc/%d/', pid
		current=current->next;
		count++;
	}	
	printf("Total backgroundjobs: %i\n\n", count);
}

//bgkill pid - TERM job w/ pid
	//indicate when job terminated (waitpid() use WNOHANG)
void bgkill(char* s){
	s=s+7;	
	pid_t cpid=atoi(s);
	kill(cpid, SIGKILL);
	//will this trigger update_bg_process() to remove from linked list?
}
	
//bgstop pid - STOP job w/ pid
void bgstop(char* s){
	s=s+7;	
	pid_t cpid=atoi(s);
	kill(cpid, SIGSTOP);
}

//bgstart pid - CONT job w/ pid
void bgstart(char* s){
	s=s+7;	
	pid_t cpid=atoi(s);
	kill(cpid, SIGCONT);
}

//3.3
//pstat pid - lists:
	//comm: filename in parentheses
	//state: R S D Z T t W X x K W or P
	//utime: time process scheduled in user mode
	//stime: time scheduled in kernel mode
	//rss: Resident Set Size
	//voluntary_ctxt_switches:
	//nonvoluntary_ctxt_switches:
	//If pid doesnt exist, return error "Error: Process 1245 does not exist."
void pstat(char* s){
	s=s+6;
	int curpid=atoi(s);
	
	char proc_list[100];		
	sprintf(proc_list, "/proc/%i/stat", curpid);
	FILE* fpstat=fopen(proc_list, "r");
	char comm[100];
	char state[2];
	state[2]='\0';
	unsigned long utime[1];
	unsigned long stime[1];
	long int rss[1];
	fscanf(fpstat, "%*d %s %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %*d %*d %*llu %*u %ld", comm, state, utime, stime, rss);
	//utime[1]=utime[1]/sysconf(_SC_CLK_TCK);
	//stime[1]=stime[1]/sysconf(_SC_CLK_TCK);
	fclose(fpstat);

	char proc_list2[100];		
	sprintf(proc_list2, "/proc/%i/status", curpid);
	FILE* fpstatus=fopen(proc_list2, "r");
	int i;
	char ignore[1024];
	for (i=0; i<39; i++)
		fgets(ignore, sizeof(ignore), fpstatus);
	char vcs[1024];
	char nvcs[1024];
	fgets(vcs, sizeof(vcs), fpstatus);	
	fgets(nvcs, sizeof(nvcs), fpstatus);
	fclose(fpstatus);

	printf("\n");
	printf("comm: %s\n", comm);
	printf("state: %s\n", state);
	printf("utime: %lu\n", utime[1]);
	printf("stime: %lu\n", stime[1]);
	printf("rss: %ld\n", rss[1]);
	printf("vcs:%s", vcs);
	printf("nvcs%s", nvcs);
	printf("\n");

}

	

int main(int argc, char* argv[]){
	
	while(1){

		update_bg_process();		

		printf("PMan: >");
		char command[10000];
		int i;
		for (i=0; i<strlen(command); i++)
			command[i]='\0';
		update_bg_process();
		fgets(command, 100000, stdin);
		update_bg_process();
		int len=strlen(command);
		if(len>0 && command[len-1]=='\n')
			command[len-1]='\0';
	

		if(strcmp(command, "bglist")==0){
			bglist();	
		}
		else if(strncmp(command, "bgkill", 6)==0){
			bgkill(command);
		}		
		else if(strncmp(command, "bgstop", 6)==0){
			bgstop(command);
		}
		else if(strncmp(command, "bgstart", 7)==0){
			bgstart(command);
		}
		else if(strncmp(command, "pstat", 5)==0){
			pstat(command);
		}
		else if(strncmp(command, "bg", 2)==0){
			bgbegin(command);
		}
		else {
			printf("command not found\n");
		}
		update_bg_process();
	}
	return 0;	
}
