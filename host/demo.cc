#include <arpa/inet.h>
#include <getopt.h>
#include <net/if.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <rte_atomic.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_flow.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>

#include "common.h"
#include "conf.h"
#include "debug.h"
#include "demo.h"
#include "env.h"
#include "inap.h"

static void
signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\n");
        RTE_LOG(INFO, APP, "Signal %d received, preparing to exit...\n", signum);
    }
    inap_terminate();
}

char    *test_data[MAX_THREAD]    = {NULL};
uint8_t *test_data_op[MAX_THREAD] = {NULL};

static void
data_generator() {
    struct conf_t *conf = get_conf();
    if (conf->is_load == 0) { // transaction
        char filename[120];
        for (int i = 0; i < conf->num_thread; i++) {
            sprintf(filename, "%s_%d.txt", conf->trace_file, (i + 1));
            FILE *file = fopen(filename, "r");
            if (file == NULL) {
                perror("open file");
                return;
            }
            size_t bytes;
            bytes           = (NUM_REQ) * sizeof(struct kv_pair_t);
            test_data[i]    = (char *)rte_zmalloc(NULL, bytes, 0);
            bytes           = (NUM_REQ) * sizeof(uint8_t);
            test_data_op[i] = (uint8_t *)rte_zmalloc(NULL, bytes, 0);

            char    line[256];
            int     loop = 0;
            uint8_t op;
            int     key, offset;
            char    command[50];
            char    value[50];

            loop = 0;
            while (fgets(line, sizeof(line), file)) {
                int result = sscanf(line, "%s %d %[^\n]", command, &key, value);

                if (loop % 1000000 == 0)
                    Dprintf("result = %d, command = %s, key = %d, value = %s\n", result, command, key, value);

                offset = loop * sizeof(struct kv_pair_t);
                rte_memcpy(test_data[i] + offset, &key, sizeof(int));
                if (result == 2) {
                    op = 0x0;
                } else {
                    op = 0x1;
                    rte_memcpy(test_data[i] + offset + 16, &value, sizeof(uint8_t) * 16);
                }
                rte_memcpy(test_data_op[i] + loop, &op, sizeof(uint8_t));
                loop++;
                if (loop == NUM_REQ) {
                    break;
                }
            }

            fclose(file);
        }
    } else { // load
        FILE *file = fopen(conf->trace_file, "r");
        if (file == NULL) {
            perror("open file");
            return;
        }
        size_t bytes;
        bytes           = (NUM_KEY) * sizeof(struct kv_pair_t);
        test_data[0]    = (char *)rte_zmalloc(NULL, bytes, 0);
        bytes           = (NUM_KEY) * sizeof(uint8_t);
        test_data_op[0] = (uint8_t *)rte_zmalloc(NULL, bytes, 0);

        char    line[256];
        int     loop = 0;
        uint8_t op;
        int     key, offset;
        char    command[50];
        char    value[50];

        loop = 0;
        op   = 0x2;
        while (fgets(line, sizeof(line), file)) {
            int result = sscanf(line, "%s %d %[^\n]", command, &key, value);
            if (loop % 1000000 == 0)
                Dprintf("result = %d, command = %s, key = %d, value = %s\n", result, command, key, value);

            offset = loop * sizeof(struct kv_pair_t);
            rte_memcpy(test_data[0] + offset, &key, sizeof(int));
            rte_memcpy(test_data[0] + offset + 16, &value, sizeof(uint8_t) * 16);
            rte_memcpy(test_data_op[0] + loop, &op, sizeof(uint8_t));
            loop++;
        }

        fclose(file);
    }
}

static void *
task_daemon(__attribute__((unused)) void *arg) {
    RTE_LOG(INFO, APP, "task_daemon start! \n");
    struct conf_t *conf = get_conf();
    int            cnt  = 0;
    set_start_time();
    while (1) {
        if (check_task() == 1) {
            cnt++;
        }
        if (cnt == conf->num_thread) {
            break;
        }
    }
    set_end_time();
    print_stats();
    return NULL;
}

void demo_init(int argc, char **argv) {
    signal(SIGINT, signal_handler);

    char *my_argv[15];
    for (int loop = 0; loop < 15; loop++) {
        my_argv[loop] = (char *)malloc(50);
    }

    int my_argc = 0;

    strcpy(my_argv[my_argc++], argv[0]);
    strcpy(my_argv[my_argc++], "-c");

    unsigned int ngx_ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    if (ngx_ncpu == 80) {
        strcpy(my_argv[my_argc++], "0xfffff00000");
    }
    strcpy(my_argv[my_argc++], "-n");
    strcpy(my_argv[my_argc++], "4");
    strcpy(my_argv[my_argc++], "--socket-mem=0,8192");

    printf("EAL: ");
    for (int loop = 0; loop < my_argc; loop++) {
        printf("%s ", my_argv[loop]);
    }
    printf("\n");

    int ret = rte_eal_init(my_argc, (char **)my_argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Could not initialise EAL (%d)\n", ret);
    printf("eal success!\n");
    unsigned nb_ports = rte_eth_dev_count_avail();
    // if (nb_ports < 2 || (nb_ports & 1))
    //     rte_exit(EXIT_FAILURE, "Error: number of ports must be even (%u detected)\n", nb_ports);
    if (rte_lcore_count() <= 1)
        rte_exit(EXIT_FAILURE, "Error: must specify at least 1 slave core\n");

    opt_parser(argc, argv);
    print_conf();

    para_init();
    print_conn();

    inap_init();
    env_init();

    struct conf_t *conf = get_conf();
    if (conf->mode == MODE_CLIENT) {
        data_generator();
        RTE_LOG(INFO, APP, "data generator success...\n");
        struct task_t task = {0};
        for (int i = 0; i < conf->num_thread; i++) {
            task.thread_id = i;
            task.addr      = test_data[i];
            task.op        = test_data_op[i];
            push_task(task);
        }
    }

    eseq = (uint8_t *)rte_zmalloc(NULL, KEY_IN_SWITCH * sizeof(uint8_t), 0);

    struct conn_arr_t *conn_arr = get_connarr();
    for (int loop = 1; loop < conn_arr->num_lcore; loop++) {
        if (conf->mode == MODE_CLIENT)
            rte_eal_remote_launch(lcore_client, &(conn_arr->conn[loop]), conn_arr->conn[loop].lcore_id);
        else
            rte_eal_remote_launch(lcore_server, &(conn_arr->conn[loop]), conn_arr->conn[loop].lcore_id);
    }

    pthread_t thread_id;
    if (conf->mode == MODE_CLIENT) {
        pthread_create(&thread_id, NULL, task_daemon, NULL);
    }
    rte_eal_mp_wait_lcore();

    if (conf->mode == MODE_CLIENT) {
        pthread_join(thread_id, NULL);
    }

    for (int loop = 0; loop < 15; loop++) {
        free(my_argv[loop]);
    }
}

void demo_exit(void) {
    env_exit();
    for (int i = 0; i < MAX_THREAD; i++) {
        if (test_data[i] != NULL)
            rte_free(test_data[i]);
        if (test_data_op[i] != NULL)
            rte_free(test_data_op[i]);
    }
}
