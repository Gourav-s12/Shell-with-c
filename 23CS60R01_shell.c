// gcc -o my_shell_vi shell_vi.c -lreadline -lcurses
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>
#define MAX_STR 200  // max command dimension
#define MAX_DIM 1000 // Maximum vector dimension

// Structure to pass data to the threads
typedef struct
{
    int num_thread;
    char cmd[MAX_STR];
    int size;
    double v1[MAX_DIM];     // First vector
    double v2[MAX_DIM];     // Second vector
    double result[MAX_DIM]; // Resultant vector
} ThreadData;
ThreadData vec_share_data;

int ch;
int row, col;
FILE *vi_file;
int no_of_line;
int no_of_word;
char str_to_put[MAX_STR];

int vi_editor_process(char filename[]);

void help_print()
{
    printf("1. pwd - shows the present working directory\n");
    printf("2. cd <directory_name> - changes the present working directory to the directory \nspecified in <directory_name>. A full path to a directory may be given.\n");
    printf("3. mkdir <directory_name> - creates a new directory specified by <dir>.\n");
    printf("4. ls <flag> - shows the contents of a directory based on some optional flags, like -ls -al, ls, etc. See man page of ls to know more about the flags.\n");
    printf("5. exit - exits the shell\n");
    printf("6. help - prints this list of commnds\n");
    printf("7. If you type any other command at the prompt, the shell should just assume it is an executable file and try to directly execute it.\n");
    exit(1);
}

void vi_command_process(char filename[])
{
    char cwd_path[MAX_STR];
    getcwd (cwd_path, MAX_STR);
    strcat(cwd_path, "/");
    strcat(cwd_path, filename);
    // printf("%s\n",cwd_path);
    if (vi_editor_process(cwd_path) != 0)
    {
        perror("execlp failed");
        exit(1);
    }
    exit(1);
}

void *vectorOperation(void *data)
{
    int pos = *((int *)data);
    int segmentSize = vec_share_data.size / vec_share_data.num_thread;
    int start = pos * segmentSize;
    int end = (pos == vec_share_data.num_thread - 1) ? vec_share_data.size : (pos + 1) * segmentSize;

    // ThreadData *threadData = (ThreadData *)data;
    for (int i = start; i < end; i++)
    {
        if (strcmp(vec_share_data.cmd, "addvec") == 0)
        {
            vec_share_data.result[i] = vec_share_data.v1[i] + vec_share_data.v2[i];
        }
        else if (strcmp(vec_share_data.cmd, "subvec") == 0)
        {
            vec_share_data.result[i] = vec_share_data.v1[i] - vec_share_data.v2[i];
        }
        else if (strcmp(vec_share_data.cmd, "dotprod") == 0)
        {
            vec_share_data.result[i] = vec_share_data.v1[i] * vec_share_data.v2[i];
        }
    }

    pthread_exit(NULL);
    // return NULL;
}

void pthread_vec_cmd(char *para[])
{
    if (para[1] == NULL || para[2] == NULL)
    {
        printf("Usage: %s <file_1> <file_2> [-<no_thread>]\n", para[0]);
        exit(1);
    }

    char *file1 = para[1];
    char *file2 = para[2];

    FILE *f1 = fopen(file1, "r");
    FILE *f2 = fopen(file2, "r");

    if (f1 == NULL || f2 == NULL)
    {
        perror("Error opening input files");
        exit(1);
    }
    strcpy(vec_share_data.cmd, para[0]);
    if (para[3] != NULL && para[3][0] == '-' && isdigit(para[3][1]))
    {
        vec_share_data.num_thread = atoi(para[3] + 1); // Skip the '-' and convert the rest to an integer
        printf("Extracted num_thread: %d\n", vec_share_data.num_thread);
    }
    else
    {
        vec_share_data.num_thread = 3;
    }

    int n = 0; // size of vector
    while (fscanf(f1, "%lf", &vec_share_data.v1[n]) == 1 && fscanf(f2, "%lf", &vec_share_data.v2[n]) == 1)
    {
        n++;
    }
    vec_share_data.size = n;

    pthread_t threads[vec_share_data.num_thread];
    int *arr = malloc(vec_share_data.num_thread * sizeof(int));

    for (int i = 0; i < vec_share_data.num_thread; i++)
    {
        arr[i] = i;
        pthread_create(&threads[i], NULL, vectorOperation, (void *)&arr[i]);
    }

    for (int i = 0; i < vec_share_data.num_thread; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("Resulting vector: \n");
    for (int i = 0; i < n; i++)
    {
        printf("%lf ", vec_share_data.result[i]);
    }
    printf("\n");
    exit(1);
}

int cmds_without_fork(char *para[])
{
    if (strcmp(para[0], "exit") == 0)
    {
        exit(1);
    }
    if (strcmp(para[0], "cd") == 0)
    {
        chdir(para[1]);
        return 1;
    }
    return 0;
}

void process(char str[])
{
    char *commands[MAX_STR];
    int no_commands = 0;
    char *parameter[MAX_STR];
    int multiprogram = 0;
    int no_parameter = 0;
    int pipe_count = 0;
    int prev_pipe[2];
    int curr_pipe[2];

    char *token_c = strtok(str, "|");
    while (token_c != NULL)
    {
        commands[no_commands++] = token_c;
        token_c = strtok(NULL, "|");
    }

    for (int i = 0; i < no_commands; i++)
    {
        if (pipe(curr_pipe) == -1)
        {
            perror("Pipe creation failed");
            exit(1);
        }
        no_parameter = 0;
        char *token_p = strtok(commands[i], " ");

        // Keep printing tokens while one of the
        // delimiters present in str[].
        while (token_p != NULL)
        {
            // printf("token %s token\n", token_p);
            parameter[no_parameter] = token_p;
            token_p = strtok(NULL, " ");
            no_parameter++;
        }
        parameter[no_parameter] = NULL;

        // handle pararal multiprogam
        if (no_parameter > 0 && parameter[no_parameter - 1][strlen(parameter[no_parameter - 1]) - 1] == '&')
        {
            parameter[no_parameter - 1][strlen(parameter[no_parameter - 1]) - 1] = '\0';
            if (strcmp(parameter[no_parameter - 1], "") == 0)
            {
                no_parameter--;
                parameter[no_parameter] = NULL;
            }
            multiprogram = 1;
        }
        if (cmds_without_fork(parameter) == 1)
        {
            return;
        }

        // handle file input output for "> <" case
        int input_redirection = 0;
        int output_redirection = 0;
        char *input_file = NULL;
        char *output_file = NULL;

        for (int i = 0; i < no_parameter; i++)
        {
            if (strcmp(parameter[i], "<") == 0)
            {
                input_redirection = 1;
                input_file = parameter[i + 1];
                parameter[i] = NULL;
            }
            else if (strcmp(parameter[i], ">") == 0)
            {
                output_redirection = 1;
                output_file = parameter[i + 1];
                parameter[i] = NULL;
            }
        }

        pid_t child_pid;

        child_pid = fork(); // Create a child process

        if (child_pid == -1)
        {
            perror("Fork failed");
            exit(1);
        }

        if (child_pid == 0)
        {
            // This code runs in the child process

            // Handle input and output redirection
            if (input_redirection)
            {
                int fd = open(input_file, O_RDONLY);
                if (fd == -1)
                {
                    perror("input redirection failed");
                    exit(1);
                }
                if (dup2(fd, STDIN_FILENO) == -1)
                {
                    perror("dup2 for input redirection failed");
                    exit(1);
                }
                close(fd);
            }

            if (output_redirection)
            {
                int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1)
                {
                    perror("output redirection failed");
                    exit(1);
                }
                if (dup2(fd, STDOUT_FILENO) == -1)
                {
                    perror("dup2 for output redirection failed");
                    exit(1);
                }
                close(fd);
            }

            // If it's not the first command, set up input redirection
            if (i > 0)
            {
                dup2(prev_pipe[0], STDIN_FILENO);
                close(prev_pipe[0]);
            }

            // If it's not the last command, set up output redirection
            if (i < no_commands - 1)
            {
                dup2(curr_pipe[1], STDOUT_FILENO);
                close(curr_pipe[1]);
            }
            // Close all pipe descriptors
            close(prev_pipe[0]);
            close(prev_pipe[1]);
            close(curr_pipe[0]);
            close(curr_pipe[1]);

            // to handle cd commend
            if (strcmp(parameter[0], "help") == 0)
            {
                help_print();
            }
            else if (strcmp(parameter[0], "vi") == 0)
            {
                vi_command_process(parameter[1]);
            }
            else if ((strcmp(parameter[0], "addvec") == 0) || (strcmp(parameter[0], "subvec") == 0) || (strcmp(parameter[0], "dotprod") == 0))
            {
                pthread_vec_cmd(parameter);
            }
            else if (execvp(parameter[0], parameter) == -1)
            {
                perror("execlp failed");
                exit(1);
            }
        }
        else
        {
            // This code runs in the parent process

            // Close the write end of the previous pipe
            if (i > 0)
            {
                close(prev_pipe[0]);
                close(prev_pipe[1]);
            }

            // Save the current pipe as the previous pipe for the next iteration
            prev_pipe[0] = curr_pipe[0];
            prev_pipe[1] = curr_pipe[1];

            // If it's not the last command in the pipeline, recursively call process for the next command
            if (multiprogram != 1)
            {
                // wait for child process to execute
                int status;
                waitpid(child_pid, &status, 0);
            }
        }
    }
}

int main()
{

    char str[MAX_STR] = "echo -- Welcome TO custom SHELL --";
    process(str); 

    while (1)
    {
         
        char *input;
        strcpy(str, "");

        // multi line / input
        while (1)
        {
            input = readline("shell>");

            input[strcspn(input, "\n")] = '\0';
            add_history(input);

            if (input == NULL || input[strlen(input) - 1] == '\\')
            {
                input[strlen(input) - 1] = '\0'; // Remove the backslash
                strcat(str, input);              
                free(input);                     
            }
            else
            {
                strcat(str, input); // Append the current line to the buffer
                free(input);        
                break;
            }
        }
        process(str); // Process the command
    }
    return 0;
}


// vi editor

int checkEndOfLine()
{
    int t_row = row;
    int t_col = col + 1;
    int c;

    // Loop through characters to the right until the end of the line or screen width
    while (t_col < COLS)
    {

        c = mvinch(t_row, t_col) & A_CHARTEXT;
        if (c == '\n' || c == '\r')
        {
            // Reached the end of the line
            return 1;
        }
        else if (!(c == ERR || c == ' '))
        {
            return 0;
        }
        t_col++;
    }

    // If we reached the screen width, consider it the end of the line
    return 1;
}
void Read_display_file()
{
    // Read and display the file
    while ((ch = fgetc(vi_file)) != EOF)
    {
        addch(ch);
        col++;
        if (col == COLS)
        {
            col = 0;
            row++;
        }
    }
}
void go_to_the_last_char_in_line()
{
    // Check if the original position is valid in the upper line
    int c = mvinch(row, col) & A_CHARTEXT;
    if (checkEndOfLine() == 1)
    {
        while ((c == ERR || c == ' ') && col > 0)
        {
            col--;
            c = mvinch(row, col) & A_CHARTEXT;
        }
        if (!(c == ERR || c == ' '))
        {
            col++;
        }
    }
}
void key_up_input()
{
    if (row > 0)
    {
        --row;
        go_to_the_last_char_in_line();

        move(row, col);
    }
}
void key_down_input()
{
    if (row < LINES - 1)
    {
        ++row;

        go_to_the_last_char_in_line();
        move(row, col);
    }
}
void key_left_input()
{
    if (col > 0)
    {
        move(row, --col);
    }
    else if (row > 0)
    {
        row--;
        col = COLS - 1;
        go_to_the_last_char_in_line();

        move(row, col);
    }
}
void key_right_input()
{
    int c = mvinch(row, col) & A_CHARTEXT;

    if (col < COLS - 1 && (!(c == ERR || c == ' ') || checkEndOfLine() == 0))
    {
        ++col;
        move(row, col);
    }
    else
    {
        row++;
        col = 0;
        move(row, col);
    }
}
void char_del_input()
{
    if (col == 0 && row == 0)
    {
        return;
    }
    key_left_input();
    delch();
}
void char_insert_input()
{
    if ((ch >= 32 && ch <= 126) )// || ch == 13) // Printable characters
    {
        insch(ch);

        if (col < COLS - 1)
        {
            move(row, ++col);
        }
        else
        {
            row++;
            col = 0;
            move(row, col);
        }
    }
}
void file_save(int modified)
{
    // Save the file
    fseek(vi_file, 0, SEEK_SET);
    wmove(stdscr, 0, 0);

    int last_non_empty_line = LINES - 2;
    no_of_word = 0;

    // Find the last non-empty line
    for (row = LINES - 2; row >= 0; row--)
    {
        col = 0;
        int c = mvinch(row, col) & A_CHARTEXT;
        // test_e++;
        if (!(c == '\n' || c == '\r' || c == ' ' || checkEndOfLine()))
        {

            last_non_empty_line = row;
            break;
        }
    }

    for (int y = 0; y <= last_non_empty_line; y++)
    {
        strcpy(str_to_put, "");
        for (int x = 0; x < COLS; x++)
        {
            int c = mvinch(y, x) & A_CHARTEXT;
            if (c != ERR)
            {
                // fputc(c, vi_file);
                // row = y;
                // col = x;
                // int ch = mvinch(y, x) & A_CHARTEXT;
                // if (ch == ' ' && (checkEndOfLine() == 1 || c < 0))
                // {
                //     fputc('\n', vi_file);
                //     break;
                // }
                row = y;
                col = x;
                char ch = (char)c;
                // if (c == '\n' || checkEndOfLine() == 1 || c < 0)
                // {
                //     break;
                // }
                strncat(str_to_put, &ch, 1);
                if ((c == ' ' && (checkEndOfLine() == 1 || c < 0)) || c == '\n' || c == '\r')
                {
                    if (x == 0)
                    {
                        no_of_word--;
                    }
                    strcat(str_to_put, "\n");
                    x = COLS;
                    break;
                }
                if (c == ' ')
                {
                    no_of_word++;
                }
            }
        }
        strcat(str_to_put, "\0");
        fputs(str_to_put, vi_file);
    }
    fclose(vi_file);
    no_of_word += last_non_empty_line + 1;
    no_of_line = last_non_empty_line + 1;
    // return str_to_put;
}
void delete_last_extra_line(const char *filename)
{
    char destinationFileName[] = "tempoutkeep.txt"; // Replace with your destination file name
    int n = no_of_line;                             // Number of lines to copy
    FILE *sourceFile, *destinationFile;
    char line[COLS];

    sourceFile = fopen(filename, "r");
    if (sourceFile == NULL)
    {
        perror("Error opening source file");
        return;
    }

    destinationFile = fopen(destinationFileName, "w");
    if (destinationFile == NULL)
    {
        perror("Error opening destination file");
        fclose(sourceFile);
        return;
    }

    for (int i = 0; i < n; i++)
    {
        if (fgets(line, sizeof(line), sourceFile) != NULL)
        {
            fputs(line, destinationFile);
        }
        else
        {
            break; // End of file reached before copying 'n' lines
        }
    }

    fclose(sourceFile);
    fclose(destinationFile);

    // Remove the original file
    if (remove(filename) != 0)
    {
        perror("Error deleting the original file");
        return;
    }

    // Rename the new file to the original file name
    if (rename(destinationFileName, filename) != 0)
    {
        perror("Error renaming the new file");
        return;
    }
}


int vi_editor_process(char filename[])
{
    if (filename == NULL)
    {
        printf("Usage: %s <filename>\n", filename);
        return 1;
    }

    // Open the file for editing
    // const char *filename = argv[1];
    vi_file = fopen(filename, "r+");
    if (vi_file == NULL)
    {
        printf("Can not open file %s : No such file or directory\n", filename);
        // perror("Can not open file %s :", argv[1]);
        return 1;
    }

    // Initialize ncurses
    initscr();
    keypad(stdscr, TRUE);
    noecho();

    // Disable Ctrl+S and Ctrl+Q to capture them
    raw();

    

    row = 0, col = 0;

    // Initialize counts
    int num_chars_modified = 0;

    Read_display_file();
    // Move the cursor to the beginning
    row = 0, col = 0;
    move(0, 0);
    int modified = 0;
    no_of_line = 0;
    no_of_word = 0;

    while (1)
    {
        ch = getch();

        switch (ch)
        {
            //toggle keys
        case KEY_UP:
            key_up_input();
            break;

        case KEY_DOWN:
            key_down_input();
            break;

        case KEY_LEFT:
            key_left_input();
            break;

        case KEY_RIGHT:
            key_right_input();
            break;

        case KEY_DC:        // Delete key
        case KEY_BACKSPACE: // Backspace key
            char_del_input();
            num_chars_modified++;
            modified = 1;
            break;

        case 27: // ESC key
            if (modified)
            {
                mvprintw(LINES - 1, 0, "Press Ctrl+S to save and exit, or Ctrl+X to exit without saving.");
                refresh();

                while (1)
                {
                    int esc_ch = getch();
                    if (esc_ch == 19) // Ctrl+S
                    {
                        file_save(modified);
                        delete_last_extra_line(filename);
                        endwin();
                        printf("Number of lines: %d \n", no_of_line);
                        printf("Number of words: %d \n", no_of_word);
                        printf("Number of characters modified: %d \n", num_chars_modified);
                        return 0;
                    }
                    else if (esc_ch == 24) // Ctrl+X
                    {
                        fclose(vi_file);
                        endwin();
                        return 0;
                    }
                }
            }
            else
            {
                fclose(vi_file);
                endwin();
                return 0;
            }

            break;
        case 19: // Ctrl+S
            file_save(modified);
            delete_last_extra_line(filename);
            endwin();
            printf("Number of lines: %d \n", no_of_line);
            printf("Number of words: %d \n", no_of_word);
            printf("Number of characters modified: %d \n", num_chars_modified);
            return 0;
            break;
        case 24: // Ctrl+X
            fclose(vi_file);
            endwin();
            return 0;
            break;
        default:
            char_insert_input();
            if ((ch >= 32 && ch <= 126) )//|| ch == 13) // Printable characters
            {
                num_chars_modified++;
                modified = 1;
            }
            break;
        }

        refresh();
    }

    endwin();
    return 0;
}