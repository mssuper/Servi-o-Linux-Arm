/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: marcelo
 *
 * Created on 30 de Junho de 2018, 13:22
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>    
#include <fcntl.h>     
#include <errno.h>     
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <syslog.h>
#include <stdarg.h>

#include <stdbool.h>

#define try bool __HadError=false;
#define catch(x) ExitJmp:if(__HadError)
#define throw(x) __HadError=true;goto ExitJmp;



#define VERSION_MAJOR_INIT 1
#define VERSION_MINOR_INIT 0




static int running = 0;
static int delay = 1;
static int counter = 0;
static char *conf_file_name = NULL;
static char *pid_file_name = NULL;
static int pid_fd = -1;
static char *app_name = NULL;
static FILE *log_stream;
static bool serial_available = 1;
static bool debug_messages = false;

char *strdup(const char *s) {
    char *d = malloc(strlen(s) + 1); // Space for length plus nul
    if (d == NULL) return NULL; // No memory
    strcpy(d, s); // Copy the characters
    return d; // Return the new string
}

int getcompilemonth() {

    if (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')
        return 1;
    if (__DATE__[0] == 'F')
        return 2;
    if (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r')
        return 3;
    if (__DATE__[0] == 'A' && __DATE__[1] == 'p')
        return 4;
    if (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y')
        return 5;
    if (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n')
        return 6;
    if (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l')
        return 7;
    if (__DATE__[0] == 'A' && __DATE__[1] == 'u')
        return 8;
    if (__DATE__[0] == 'S')
        return 9;
    if (__DATE__[0] == 'O')
        return 10;
    if (__DATE__[0] == 'N')
        return 11;
    if (__DATE__[0] == 'D')
        return 12;
}

char *make_version() {

    char buffer[32];

    char m[2], c[2], d[2], u[2], hour[3], min[3], sec[3], day[3];
    m[0] = __DATE__[7];
    m[1] = 0;
    c[0] = __DATE__[8];
    c[1] = 0;
    d[0] = __DATE__[9];
    d[1] = 0;
    u[0] = __DATE__[10];
    u[1] = 0;
    hour[0] = __TIME__[0];
    hour[1] = __TIME__[1];
    hour[2] = 0;
    min[0] = __TIME__[3];
    min[1] = __TIME__[4];
    min[2] = 0;
    sec[0] = __TIME__[6];
    sec[1] = __TIME__[7];
    sec[2] = 0;
    day[0] = __DATE__[4];
    day[1] = __DATE__[5];
    day[2] = 0;


    sprintf(buffer, "%d.%d.%s%s%s%s.%d.%s%s%s", VERSION_MAJOR_INIT, VERSION_MINOR_INIT, m, c, d, u, getcompilemonth(), hour, min, sec);
    return strdup(buffer);
}

void version(void) {
    printf("\nVersão do Sistema: %s\n\n", make_version());
}

/**
 * \brief Read configuration from config file
 */
int read_conf_file(int reload) {
    FILE *conf_file = NULL;
    int ret = -1;

    if (conf_file_name == NULL) return 0;

    conf_file = fopen(conf_file_name, "r");

    if (conf_file == NULL) {
        syslog(LOG_ERR, "Não foi possivel abrir o arquivo de configuração: %s, error: %s",
                conf_file_name, strerror(errno));
        return -1;
    }

    ret = fscanf(conf_file, "%d", &delay);

    if (ret > 0) {
        if (reload == 1) {
            syslog(LOG_INFO, "Reloaded configuration file %s of %s",
                    conf_file_name,
                    app_name);
        } else {
            syslog(LOG_INFO, "Configuration of %s read from file %s",
                    app_name,
                    conf_file_name);
        }
    }

    fclose(conf_file);

    return ret;
}

/**
 * \brief This function tries to test config file
 */
int test_conf_file(char *_conf_file_name) {
    FILE *conf_file = NULL;
    int ret = -1;

    conf_file = fopen(_conf_file_name, "r");

    if (conf_file == NULL) {
        fprintf(stderr, "Não foi possível ler o arquivo de configuração %s\n",
                _conf_file_name);
        return EXIT_FAILURE;
    }

    ret = fscanf(conf_file, "%d", &delay);

    if (ret <= 0) {
        fprintf(stderr, "Arquivo de configuração incorreto %s\n",
                _conf_file_name);
    }

    fclose(conf_file);

    if (ret > 0)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}

/**
 * \brief Callback function for handling signals.
 * \param	sig	identifier of signal
 */
void handle_signal(int sig) {
    if (sig == SIGINT) {
        fprintf(log_stream, "Debug: parando serviço ...\n");
        /* Unlock and close lockfile */
        if (pid_fd != -1) {
            lockf(pid_fd, F_ULOCK, 0);
            close(pid_fd);
        }
        /* Try to delete lockfile */
        if (pid_file_name != NULL) {
            unlink(pid_file_name);
        }
        running = 0;
        /* Reset signal handling to default behavior */
        signal(SIGINT, SIG_DFL);
    } else if (sig == SIGHUP) {
        fprintf(log_stream, "Debug: recarregando arquivo de configuração do serviço...\n");
        read_conf_file(1);
    } else if (sig == SIGCHLD) {
        fprintf(log_stream, "Debug: Sinall SIGCHLD recebido\n");
    }
}

/**
 * \brief This function will to_service this app
 */
static void to_service() {
    pid_t pid = 0;
    int fd;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    /* Success: Let the parent terminate */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* On success: The child process becomes session leader */
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    /* Ignore signal sent from child to parent process */
    signal(SIGCHLD, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    /* Success: Let the parent terminate */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/ajm");

    /* Close all open file descriptors */
    for (fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--) {
        close(fd);
    }

    /* Reopen stdin (fd = 0), stdout (fd = 1), stderr (fd = 2) */
    stdin = fopen("/dev/null", "r");
    stdout = fopen("/dev/null", "w+");
    stderr = fopen("/dev/null", "w+");

    /* Try to write PID of daemon to lockfile */
    if (pid_file_name != NULL) {
        char str[256];
        pid_fd = open(pid_file_name, O_RDWR | O_CREAT, 0640);
        if (pid_fd < 0) {
            /* Can't open lockfile */
            exit(EXIT_FAILURE);
        }
        if (lockf(pid_fd, F_TLOCK, 0) < 0) {
            /* Can't lock file */
            exit(EXIT_FAILURE);
        }
        /* Get current PID */
        sprintf(str, "%d\n", getpid());
        /* Write PID to lockfile */
        write(pid_fd, str, strlen(str));
    }
}

/**
 * \brief Print help for this application
 */
void print_help(void) {
    printf("\n Uso: %s [OPÇÕES]\n\n", app_name);
    printf("  Opções:\n");
    printf("   -v --version       Versão do arquivo\n");
    printf("   -h --help          Imprime esta tela\n");
    printf("   -c --conf_file     Le a configuração do arquivo\n");
    printf("   -t --test_conf     Testa o arquivo de configuração\n");
    printf("   -l --log_file      Escreve logs para o arquivo\n");
    printf("   -s --service       Executar como serviço\n");
    printf("   -d --debug         Exibe mensagens de depuração\n");
    printf("   -p --pid_file      Arquivo PID usado pelo serviço\n");
    printf("\n");
}

/* Main function */
int main(int argc, char *argv[]) {
    static struct option long_options[] = {
        {"conf_file", required_argument, 0, 'c'},
        {"test_conf", required_argument, 0, 't'},
        {"log_file", required_argument, 0, 'l'},
        {"help", no_argument, 0, 'h'},
        {"debug", no_argument, 0, 'd'},
        {"service", no_argument, 0, 's'},
        {"pid_file", required_argument, 0, 'p'},
        {"version", no_argument, 0, 'v'},
        {NULL, 0, 0, 0}
    };
    char txBuffer[10];
    char rxBuffer[10];
    
    
    int value, option_index = 0, ret;
    char *log_file_name = NULL;
    int start_service = 0;
    bool reception;
    char datareceived[32];
    int countchar;
    bool data_ready = false;


    app_name = argv[0];



    /* Try to process all command line arguments */
    while ((value = getopt_long(argc, argv, "c:l:p:t:dshv", long_options, &option_index)) != -1) {
        switch (value) {
            case 'c':
                conf_file_name = strdup(optarg);
                if (debug_messages)
                    printf("\nDebug message: %s: %s", "conf_file_name\n", conf_file_name);
                break;
            case 'l':
                log_file_name = strdup(optarg);
                if (debug_messages)
                    printf("\nDebug message: %s: %s", "logfilename\n", log_file_name);
                break;
            case 'p':
                pid_file_name = strdup(optarg);
                if (debug_messages)
                    printf("\nDebug message: %s: %s", "pid_file_name\n", pid_file_name);
                break;
            case 't':
                return test_conf_file(optarg);
            case 's':
                start_service = 1;
                if (debug_messages)
                    printf("\nDebug message: %s: %d", "start_service\n", start_service);
                break;
            case 'd':
                debug_messages = true;
                break;
            case 'h':
                print_help();
                return EXIT_SUCCESS;
            case 'v':
                version();
                return EXIT_SUCCESS;
            case '?':
                print_help();
                return EXIT_FAILURE;
            default:
                break;
        }
    }
    /* When daemonizing is requested at command line. */
    if (start_service == 1) {
        /* It is also possible to use glibc function deamon()
         * at this point, but it is useful to customize your daemon. */
        to_service();
    }

    /* Open system log and write message to it */
    openlog(argv[0], LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "Iniciado %s", app_name);

    /* Daemon will handle two signals */
    signal(SIGINT, handle_signal);
    signal(SIGHUP, handle_signal);

    /* Try to open log file to this daemon */
    if (log_file_name != NULL) {
        log_stream = fopen(log_file_name, "wa+");
        if (debug_messages)
            printf("\nDebug message: %s: %s", "log_stream\n", log_file_name);
        if (log_stream == NULL) {
            syslog(LOG_ERR, "Não foi possivel abrir o arquivo de log: %s, error: %s",
                    log_file_name, strerror(errno));
            log_stream = stdout;
        }
    } else {
        log_stream = stdout;
    }

    /* Read configuration from config file */
    read_conf_file(0);

    /* This global variable can be changed in function handling signal */
    running = 1;

    /* Never ending loop of server */
    while (running == 1) {

        if (ret < 0) {
            syslog(LOG_ERR, "Não foi possível gravar no fluxo de logs: %s, error: %s",
                    (log_stream == stdout) ? "stdout" : log_file_name, strerror(errno));
            break;
        }
        ret = fflush(log_stream);
        if (ret != 0) {
            syslog(LOG_ERR, "Não foi possível limpar o fluxo de log: %s, error: %s",
                    (log_stream == stdout) ? "stdout" : log_file_name, strerror(errno));
            break;
        }

        /* Debug print */
        //ret = fprintf(log_stream, "Debug: %d\n", counter++);

        /* TODO: dome something useful here */
        // Read a char
        //



        /* Real server should use select() or poll() for waiting at
         * asynchronous event. Note: sleep() is interrupted, when
         * signal is received. */
        sleep(delay);
    }

    /* Close log file, when it is used. */
    if (log_stream != stdout) {
        fprintf(log_stream, "Parado %s", app_name);
        fclose(log_stream);
    }

    /* Write system log and close it. */
    syslog(LOG_INFO, "Parado %s", app_name);
    closelog();

    /* Free allocated memory */
    if (conf_file_name != NULL) free(conf_file_name);
    if (log_file_name != NULL) free(log_file_name);
    if (pid_file_name != NULL) free(pid_file_name);
    ret = remove(pid_file_name);//retorna erro 0x0000002 se não houver sucesso

    if (ret == 0) {
        return EXIT_SUCCESS;
    } else {
        return 0x0000002;
    }

}
