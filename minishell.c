#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LIMIT 1024
#define NOPTIONS 6 //options 
#define NCOMMANDS 5 //options 

struct Command {   // Structure declaration
  char *options[NOPTIONS];       // Member (char variable)
};

void add_space_before_semicolon(char* str) {
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (str[i] == ';') {
            // move all characters from the semicolon to the end of the string by one position
            for (int j = len; j > i; j--) {
                str[j] = str[j - 1];
            }
            // add a space before the semicolon
            str[i] = ' ';
            len++; // update the length of the string
        }
    }
}


/// @brief  
/// @param cmd command string
/// @param bError marks error in the command
/// @param commandCount count of number of commands to merge
/// @param lstCommands  list of commands to merge
/// @param lstConnectors list of all the connectors
void parseInput(char* cmd, int* bError, int* commandCount, struct Command* lstCommands, char** lstConnectors)
{
    // add_space_before_semicolon(cmd);
    // seprate the commands in to tokens by space
    char* token = strtok(cmd, " \n");
        int argsCount = 0;
        while( token != NULL ) {

            if(
                (strcmp(token, "|") == 0) ||
                (strcmp(token, ">") == 0) ||
                (strcmp(token, ">>") == 0) ||
                (strcmp(token, "<") == 0) ||
                (strcmp(token, "&&") == 0) ||
                (strcmp(token, "||") == 0) ||
                (strcmp(token, "&") == 0) ||
                (strcmp(token, ";") == 0) 
            )
            {
                lstCommands[*commandCount].options[argsCount] = NULL;
                //new command
                argsCount = 0;
                lstConnectors [(*commandCount)] = token;
                (*commandCount)= 1 + (*commandCount);  
                token = strtok(NULL, " \n");
                continue;
            }

            // create a new structure and add it to lst of commands for each command
            if(argsCount == 0)
            {
                struct Command currentCommand;
                lstCommands[*commandCount] = currentCommand;
            }

            if(argsCount > NOPTIONS)
            {
                printf("\nError: Invalid number of Arguments to a command \n");
                printf("\nThe argc of any individual command or program should be >=1 and < =6 \n");
                *bError = 1;
                break;
            }
            if(*commandCount > NCOMMANDS)
            {
                printf("\nError: Invalid number of Commands to execute \n");
                printf("\nThe argc of induvial commands or programs that are");
                printf("used along with the special characters listed below should be >=1 and <=6 \n");
                *bError = 1;
                break;
            }
            lstCommands[*commandCount].options[argsCount] = token;
            argsCount++;
            token = strtok(NULL, " \n");
        }
        lstCommands[*commandCount].options[argsCount] = NULL;
}

/// @brief 
/// @param index of the command that is being executed 
/// @param lstCommands list of commands to be merged
/// @param lstConnectors list of all the connectors
/// @param input input file descriptor 
/// @param output output file descriptor 
/// @param bwait if 1 ask parent to wait or dont wait
/// @return 
int executeSingleCommand(int index, struct Command* lstCommands, char** lstConnectors, int input, int output, int bwait)
{
    // for a child and execute a single command with described input out put file descriptor
    
    int status;
    int pid = fork();
    if(pid == 0)
    {
        dup2(input,0);
        dup2(output,1);
        // printf("executinng %d command: %s \n",index, lstCommands[index].options[0]);
        if(execvp(lstCommands[index].options[0],
            lstCommands[index].options) == -1)
        {
            printf("\n MiniShell$: Invalid Command \n");
        }
        exit(EXIT_FAILURE); 
    }
    else if(pid < 0) {
            printf("MiniShell$: Fork error\n");
            return 1;
    }
    else {
        if(bwait != 0)
        {
            // wait for the child command to execute in parent
            do {
                waitpid(pid, &status, WUNTRACED);
            } while(!WIFEXITED(status) && !WIFSIGNALED(status));

            // printf("\n child executed sucessfully %s", lstCommands[index].options[0]);
            // printf("Status %d \n", status);
            if((status == 0))
            {
                return 0;
            }else
            {
                return 1;
            }
        }
        return 0;
    }
}
/// @brief 
/// @param commandCount number of commands to be execute
/// @param lstCommands list of commands to be execute
/// @param lstConnectors list of commectors to merge imput-output
void executeAllCommands(int* commandCount, struct Command* lstCommands, char** lstConnectors)
{
    int count = 0;
    int connectorCount = 0;
    int input = 0;
    int output = 1;
    while(count <= *commandCount)
    {
        //execute last command
        if(count ==  *commandCount)
        {
            int bSucsessfull =  executeSingleCommand(count, lstCommands, lstConnectors,input,output,1);
            count++;
        }
        // update input output file for pipe and execute the commands
        if (strcmp(lstConnectors[connectorCount], "|") == 0) 
        {
             int fd[2];
            if(pipe(fd)==-1)exit(1);

            output = dup(fd[1]); 
            int bSucsessfull =  executeSingleCommand(count, lstCommands, lstConnectors,input,output,1);
            close(output);

            input = dup(fd[0]);

            close(fd[0]);
            close(fd[1]);
            output = 1;
            count++;
            connectorCount++;
        }
        // update input output files descripted for redirect and append and execute the command
        else if (strcmp(lstConnectors[connectorCount], ">") == 0 ||
                strcmp(lstConnectors[connectorCount], ">>") == 0) 
        {
            // file writing
        
                FILE *out_file;

            if(lstCommands[count + 1].options[0] != NULL)
            {
                int is_append = 0;
                if(strcmp(lstConnectors[connectorCount], ">>") == 0)
                {
                    is_append = 1;
                }
                printf("Redirect output to file");
                if(is_append == 1){
                    out_file = fopen(lstCommands[count + 1].options[0], "a");
                }else {
                    out_file = fopen(lstCommands[count + 1].options[0], "w");
                }
            }
            
             if (out_file != NULL) {
            output = fileno(out_file);}

            int bSucsessfull =  executeSingleCommand(count, lstCommands, lstConnectors,input,output,1);
            
            fclose(out_file);    
            close(output);


            output = 1;
            count++;
            count++;
            connectorCount++;
        }
        // update input output descriptoir for the input redirect and execute the command
        else if (strcmp(lstConnectors[connectorCount], "<") == 0) 
        {
            // file writing
                FILE *in_file;

            if(lstCommands[count + 1].options[0] != NULL)
            {
               in_file = fopen(lstCommands[count + 1].options[0], "r");
            }
            
             if (in_file != NULL) {
            input = fileno(in_file);}

            int bSucsessfull =  executeSingleCommand(count, lstCommands, lstConnectors,input,output,1);
            
            fclose(in_file);    
            close(input);


            input = 0;
            count++;
            count++;
            connectorCount++;
        }
        // check if the command is executed sucessfully  and if true continue or break
        else if (strcmp(lstConnectors[connectorCount], "&&") == 0) 
        {
            int bSucsessfull =  executeSingleCommand(count, lstCommands, lstConnectors,input,output,1);
            count++;
            connectorCount++;
            if(bSucsessfull == 0)
            {
                continue;
            }else
            {
                break;
            }
        }
        // check if the command is executed sucessfully  and if false continue or break
        else if (strcmp(lstConnectors[connectorCount], "||") == 0) 
        {
         int bSucsessfull =  executeSingleCommand(count, lstCommands, lstConnectors,input,output,1);
            count++;
            connectorCount++;
            if(bSucsessfull == 0)
            {
                break;
            }else
            {
                continue;
            }
        }
        // update the output to pipe output insted of standard out put and 
        //parent should not wait for command to execute
        else if (strcmp(lstConnectors[connectorCount], "&") == 0) 
        {
        // process in background

         int fd[2];
            if(pipe(fd)==-1)exit(1);

            output = dup(fd[1]);
            
            int bSucsessfull =  executeSingleCommand(count, lstCommands, lstConnectors,input,output,0);
            close(output);

            close(fd[0]);
            close(fd[1]);
            output = 1;

            count++;
            connectorCount++;
        }
        // execute commands one by one 
        else if (strcmp(lstConnectors[connectorCount], ";") == 0) 
        {
           int bSucsessfull =  executeSingleCommand(count, lstCommands, lstConnectors,input,output,1);
            count++;
            connectorCount++;
        }
    }
}
int main(void) {
    char cmd[MAX_LIMIT];
    printf("\n*****#####***** || Wellcome To Mini Shell || *****#####*****\n");
    while(1) 
    {
        //declaring data structure to store input in the form of list of commands
        struct Command lstCommands[NCOMMANDS];
        char *lstConnectors [NCOMMANDS] = {"a","b","c","d","e"};
        int commandCount = 0;
        int bError = 0;

        // take input
        printf("\n MiniShell$ ");
        fgets(cmd, MAX_LIMIT, stdin);
        
        //parse input
        parseInput(cmd, &bError, &commandCount, lstCommands, lstConnectors);
        //if error continue to new input
        if(bError)
            continue;

        //execution of commands
         executeAllCommands(&commandCount, lstCommands, lstConnectors);
    }
}