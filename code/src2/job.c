#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "job.h"
#include "StringManipulator.h"
#include "Helper.h"

void jobs_init(){
	// semaphore chnged between prent and child
	sem_init(&jobs_mutex, 0, 1);
	sem_init(&batch_jobs_mutex, 0, 1);
}

void addBatchJob(Job *newJob){
	sem_wait(&batch_jobs_mutex);

	numberOfBatchJobs++;
	jobs = (Job*)malloc( numberOfBatchJobs * sizeof(Job) );

	if(numberOfBatchJobs > 1){
		if( timeBiggerThan( &jobs[numberOfBatchJobs - 2].dateTime, &((Job*)newJob)->dateTime) ){
			jobs[numberOfBatchJobs - 1] = *(Job*)newJob;
		}else{
			jobs[numberOfBatchJobs - 1] = jobs[numberOfBatchJobs - 2];
		}
	}else{
		jobs[numberOfBatchJobs - 1] = *newJob;
	}

	sem_post(&batch_jobs_mutex);
}

void removeTopJob(){
	sem_wait(&batch_jobs_mutex);

	numberOfBatchJobs--;
	jobs = (Job*)realloc( jobs,  numberOfBatchJobs * sizeof(Job) );

	sem_post(&batch_jobs_mutex);
}

void addJob(Job *newJob){
	sem_wait(&jobs_mutex);
	Job *j = newJob;
	
	numberOfJobs++;
	j->jid = numberOfJobs;

	FILE *f;

	if( !(f=fopen(JOBS_FILENAME, "a")) ) {
    error("Error opening file.");
	}

	char jobString[STRING_BUFFER_SIZE];
	jobToString(newJob, jobString);
	strncat(jobString, "\r\n", STRING_BUFFER_SIZE);
	fputs(jobString, f);

	fclose(f);

	sem_post(&jobs_mutex);
}

void changeJob(Job *job){
	sem_wait(&jobs_mutex);
	// read file line by line until jid is found
	// output each line to temp, unless jid matches jobID, then change it and write it
	char *temporaryFileName = "temp.txt";

	FILE *f;
	FILE *temp;
	if( !(f=fopen(JOBS_FILENAME, "r")) ) {
    error("Error opening file");
	}
	if( !(temp=fopen(temporaryFileName, "w")) ) {
    error("Error opening file");
	}

	char line [STRING_BUFFER_SIZE];
	char line2[STRING_BUFFER_SIZE];
	while ( fgets ( line, STRING_BUFFER_SIZE, f ) ){
		strncpy(line2, line, STRING_BUFFER_SIZE);
		Job j = stringToJob(line);
		if(j.jid == job->jid){
			jobToString(job, line2);
		}
		fputs(line2, temp);
	}
	fputs("\n", temp);
	fclose(f);
	fclose (temp);

	remove(JOBS_FILENAME);
	rename(temporaryFileName, JOBS_FILENAME);

	sem_post(&jobs_mutex);
}

void *getJob(void *jid){}

void jobs_finish(){
	sem_destroy(&batch_jobs_mutex);
	sem_destroy(&jobs_mutex);
}

Job createJobPid(int pid, char *host, char *command, Type type, JobState state, struct tm *dateTime){
	Job j;
	j.pid = pid;
	j.jid = 0;
	strncpy(j.host, host, STRING_BUFFER_SIZE);
	strncpy(j.command, command, STRING_BUFFER_SIZE);
	j.type = type;
	j.state = state;
	j.dateTime = *dateTime;

	return j;
}

Job createJob(char *host, char *command, Type type, JobState state, struct tm *dateTime){
	return createJobPid(0, host, command, type, state, dateTime);
}

Job createJobNowPid(int pid, char *host, char *command, Type type, JobState state){
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);

	return createJobPid(pid, host, command, type, state, lt);
}

Job createJobNow(char *host, char *command, Type type, JobState state){
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);

	lt->tm_year += 1900;

	return createJob(host, command, type, state, lt);
}

Job stringToJob(char *string){
	Job j;
	char type;
	char status;
	sscanf(string, "%d %d %s %c %c %d/%d/%d %d:%d:%d",
		&j.pid,
		&j.jid,
		j.host,
		&status,
		&type,
		&j.dateTime.tm_mday, &j.dateTime.tm_mon, &j.dateTime.tm_year,
		&j.dateTime.tm_hour, &j.dateTime.tm_min, &j.dateTime.tm_sec);

	switch(status){
		case 'W': j.state = WAITING; break;
		case 'R': j.state = RUNNING; break;
		case 'T': j.state = TERMINATED; break;
		case 'F': j.state = FINISHED; break;
		default: break;
	}

	switch(type){
		case 'I': j.type = INTERACTIVE; break;
		case 'B': j.type = BATCH; break;
		default: break;
	}

	// extracting command
	char *buffers[STRING_BUFFER_AMOUNT];
	splitStringBy(string, " ", buffers, STRING_BUFFER_AMOUNT);
	// remove all except command
	for(int i = 0; i < 7; ++i){
		shiftStrings(buffers);
	}

	char command[STRING_BUFFER_SIZE];
	concatenteStrings(buffers, command, STRING_BUFFER_SIZE);
	strncpy(j.command, command, STRING_BUFFER_SIZE);

	return j;
}

void jobToString(Job *j, char *stringBuffer){
	Type type;
	JobState status;

	switch(j->state){
		case WAITING: status = 'W'; break;
		case  RUNNING: status  = 'R'; break;
		case  TERMINATED: status = 'T'; break;
		case  FINISHED: status = 'F'; break;
		default: break;
	}

	switch(j->type){
		case INTERACTIVE: type = 'I'; break;
		case BATCH: type = 'B'; break;
		default: break;
	}

	char pid_jid[16];
	sprintf(pid_jid, "%d %d ",
		j->pid,
		j->jid);
	
	char status_type_dateTime[32];
	sprintf(status_type_dateTime, " %c %c %d/%d/%d %d:%d:%d ", 
		status,
		type,
		j->dateTime.tm_mday, j->dateTime.tm_mon, j->dateTime.tm_year,
		j->dateTime.tm_hour, j->dateTime.tm_min, j->dateTime.tm_sec);
	
	strncpy(stringBuffer, pid_jid, STRING_BUFFER_SIZE);
	strncat(stringBuffer, j->host, STRING_BUFFER_SIZE);
	strncat(stringBuffer, status_type_dateTime, STRING_BUFFER_SIZE);
	strncat(stringBuffer, j->command, STRING_BUFFER_SIZE);
}