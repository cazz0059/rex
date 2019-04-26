#include "commandsManager.h"

void forkChild(char **paths, char **args, int bufferSize, bool wait){
  pid_t pid = fork();

  if (pid > 0) {
  // wait for child
  int status;
  waitpid(pid, &status, WUNTRACED);
      //addProcess(pid);
  }else if (pid == 0) {
    //Ignore SIGINT and SIGTSTP
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    //Go through each path one by one
    for (int i = 0; paths[i] != NULL; i++) {

      char pathtmp[bufferSize];
      strncpy(pathtmp, paths[i], bufferSize);
      strncat(pathtmp, "/", bufferSize);
      strncat(pathtmp, args[0], bufferSize);

      execv(pathtmp, args);
    }
    perror("Error");
    exit(-1);
  } else {
      fprintf(stderr, "Fork Failed");
  }
}

bool changeCWD(char* newDir){
  //Change the directory
  if (chdir(newDir) != 0) {
    fprintf(stderr, "Invalid path\n");
    return false;
  } else {
    return true;
  }
}

void clientRun(int sockfd, char *message, char *buffer, int bufferSize){
  strncpy(buffer, "run", bufferSize);
  strncat(buffer, " ", bufferSize);
  strncat(buffer, message, bufferSize);

  printf("\n%s\n", buffer);

  // Send message to the server
  int n;
  if ((n = write(sockfd,buffer, bufferSize)) < 0){
    perror("ERROR writing to socket");
    close(sockfd);
    exit(1);
  }
  close(sockfd);
}