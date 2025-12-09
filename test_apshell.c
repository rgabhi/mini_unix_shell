#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#define SHELL_PATH "./apshell"
#define TEST_FILE "test_output.txt"
#define BUF_SIZE 4096

// --- Helper Macros for Color ---
#define COLOR_GREEN "\033[1;32m"
#define COLOR_RED   "\033[1;31m"
#define COLOR_RESET "\033[0m"

void print_result(const char *test_name, int success, const char *msg) {
    if (success) {
        printf("%s[PASS] %s%s\n", COLOR_GREEN, test_name, COLOR_RESET);
    } else {
        printf("%s[FAIL] %s%s\n", COLOR_RED, test_name, COLOR_RESET);
        if (msg) printf("   -> %s\n", msg);
    }
}

// --- Core Function: Run Shell and Capture Output ---
// Spawns ./apshell, sends 'input_cmd' to it, and reads stdout into 'output_buf'
void run_shell_test(const char *input_cmd, char *output_buf) {
    int pipe_in[2];  // Parent -> Child (stdin)
    int pipe_out[2]; // Child -> Parent (stdout)

    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == 0) {
        // --- CHILD PROCESS (The Shell) ---
        
        // Redirect stdin to read from pipe_in
        dup2(pipe_in[0], STDIN_FILENO);
        
        // Redirect stdout to write to pipe_out
        dup2(pipe_out[1], STDOUT_FILENO);
        
        // Redirect stderr to stdout (to capture errors too)
        dup2(pipe_out[1], STDERR_FILENO);

        // Close unused pipe ends
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);

        // Execute the shell
        execl(SHELL_PATH, SHELL_PATH, NULL);
        perror("execl failed"); // Should not reach here
        exit(1);
    } else {
        // --- PARENT PROCESS (The Tester) ---

        // Close unused ends
        close(pipe_in[0]); // We don't read from input pipe
        close(pipe_out[1]); // We don't write to output pipe

        // 1. Send commands to the shell
        write(pipe_in[1], input_cmd, strlen(input_cmd));
        
        // Ensure we send an exit command so the shell terminates
        const char *exit_cmd = "\nexit\n";
        write(pipe_in[1], exit_cmd, strlen(exit_cmd));
        
        // Close write end to signal EOF to child
        close(pipe_in[1]);

        // 2. Read output from the shell
        memset(output_buf, 0, BUF_SIZE);
        int total_read = 0;
        int n;
        while ((n = read(pipe_out[0], output_buf + total_read, BUF_SIZE - total_read - 1)) > 0) {
            total_read += n;
        }
        close(pipe_out[0]);

        // 3. Wait for child to finish
        waitpid(pid, NULL, 0);
    }
}

// --- Test Cases ---

void test_basic_execution() {
    char buf[BUF_SIZE];
    run_shell_test("echo hello_world", buf);
    print_result("Basic Execution", strstr(buf, "hello_world") != NULL, "Did not find 'hello_world'");
}

void test_quoted_strings() {
    char buf[BUF_SIZE];
    // Check if quotes are removed and string is preserved
    run_shell_test("echo \"hello world\"", buf);
    
    int success = (strstr(buf, "hello world") != NULL) && (strstr(buf, "\"hello world\"") == NULL);
    print_result("Quoted Strings", success, "Quotes might not be parsed correctly");
}

void test_redirection_out() {
    char buf[BUF_SIZE];
    
    // Remove file if exists
    remove(TEST_FILE);
    
    run_shell_test("echo secret_data > " TEST_FILE, buf);
    
    // Check file content
    FILE *f = fopen(TEST_FILE, "r");
    if (f) {
        char file_content[100];
        fgets(file_content, sizeof(file_content), f);
        fclose(f);
        print_result("Redirection (>)", strstr(file_content, "secret_data") != NULL, "File content incorrect");
    } else {
        print_result("Redirection (>)", 0, "Output file was not created");
    }
    remove(TEST_FILE);
}

void test_redirection_in() {
    char buf[BUF_SIZE];
    
    // Create input file
    FILE *f = fopen(TEST_FILE, "w");
    fprintf(f, "input_test_data");
    fclose(f);
    
    run_shell_test("cat < " TEST_FILE, buf);
    
    print_result("Redirection (<)", strstr(buf, "input_test_data") != NULL, "Did not read from file");
    remove(TEST_FILE);
}

void test_pipeline() {
    char buf[BUF_SIZE];
    // echo "hello" (6 chars with newline) | wc -c
    run_shell_test("echo hello | wc -c", buf);
    // wc -c usually outputs "6"
    print_result("Pipeline (|)", strstr(buf, "6") != NULL, "Pipeline didn't return character count");
}

void test_cd() {
    char buf[BUF_SIZE];
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    
    // 1. Get the parent directory path string manually
    //    (So we know what to look for)
    char *last_slash = strrchr(cwd, '/');
    if (last_slash) *last_slash = '\0'; // Strip the last folder name
    char *parent_dir = cwd;

    // 2. Send "cd .." AND "pwd" to the shell in one session
    //    We check if the SHELL'S output contains the new path.
    run_shell_test("cd ..\npwd", buf);
    
    // 3. Verify the SHELL printed the parent directory
    int success = (strstr(buf, parent_dir) != NULL);

    print_result("Built-in (cd)", success, "Shell did not report the new directory");
}

void test_history() {
    char buf[BUF_SIZE];
    run_shell_test("echo 1\necho 2\nhistory", buf);
    
    // Check for "echo 1" and the word "history" in the output
    int success = (strstr(buf, "echo 1") != NULL) && (strstr(buf, "history") != NULL);
    print_result("Built-in (history)", success, "History output missing");
}

void test_logical_and() {
    char buf[BUF_SIZE];
    run_shell_test("echo A && echo B", buf);
    
    int success = (strstr(buf, "A") != NULL) && (strstr(buf, "B") != NULL);
    print_result("Logical AND (&&)", success, "Did not execute both commands");
}

void test_export() {
    char buf[BUF_SIZE];
    // Export a variable, then spawn a shell to print it
    run_shell_test("export MYTEST=123\nsh -c 'echo $MYTEST'", buf);
    
    print_result("Export", strstr(buf, "123") != NULL, "Environment variable not persisted");
}

int main() {
    printf("========================================\n");
    printf("     AP_SHELL C TEST SUITE              \n");
    printf("========================================\n");

    // Ensure the shell is compiled
    if (access(SHELL_PATH, F_OK) == -1) {
        printf("%sError: %s not found. Please compile your shell first.%s\n", COLOR_RED, SHELL_PATH, COLOR_RESET);
        printf("Run: make\n");
        return 1;
    }

    test_basic_execution();
    test_quoted_strings();
    test_redirection_out();
    test_redirection_in();
    test_pipeline();
    test_cd();
    test_history();
    test_logical_and();
    test_export();

    printf("\n========================================\n");
    return 0;
}