/*
 *	Set property on an ecoflow device
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <getopt.h>
#include "ecolib.h"

static void usage(void)
{
    fprintf(stderr, "ecoset [-d device] cmd {value}\n");
    exit(EXIT_FAILURE);
}

void post_message(struct device *dev, char *m)
{
    char *ptr[16];
    char appstr[128];
    char idstr[128];
    char propstr[128];

    snprintf(appstr, sizeof(appstr), "app-%s", dev->user);
    snprintf(idstr, sizeof(idstr), "%s_%s", dev->ident, dev->id);
    snprintf(propstr, sizeof(propstr), "/app/%s/%s/thing/property/set",
        dev->id, dev->serial);

    ptr[0] = "mosquitto_pub";
    ptr[1] = "-h";
    ptr[2] = "mqtt-e.ecoflow.com";
    ptr[3] = "-p";
    ptr[4] = "8883";
    ptr[5] = "-u";
    ptr[6] = appstr;
    ptr[7] = "-P";
    ptr[8] = dev->pass;
    ptr[9] = "-i";
    ptr[10] = idstr;
    ptr[11] = "-t";
    ptr[12] = propstr;
    ptr[13] = "-m";
    ptr[14] = m;
    ptr[15] = NULL;
//    execvp("echo", ptr);
    execvp("mosquitto_pub", ptr);
    perror("mosquitto_pub");
}

struct cmdmsg {
    const char *cmd;
    const char *msg;
    unsigned flags;
#define FL_WITH_INT	1	/* User provided integer value */
};

struct cmdmsg cmdlist[] = {
    { "reserve", "{\"from\":\"Android\",\"id\":\"%s\",\"isMatter\":0,\"moduleType\":1,\"operateType\":\"watthConfig\",\"params\":{\"isConfig\":1,\"bpPowerSoc\":30,\"minDsgSoc\":0,\"minChgSoc\":0},\"version\":\"1.0\"}", 0},
    { "noreserve", "{\"from\":\"Android\",\"id\":\"%s\",\"isMatter\":0,\"moduleType\":1,\"operateType\":\"watthConfig\",\"params\":{\"isConfig\":0,\"bpPowerSoc\":30,\"minDsgSoc\":0,\"minChgSoc\":0},\"version\":\"1.0\"}", 0},
    { NULL, "" }
};
    
struct cmdmsg *find(const char *cmd)
{
    struct cmdmsg *msg = cmdlist;

    while(msg->cmd) {
        if (strcmp(msg->cmd, cmd) == 0)
            return msg;
        msg++;
    }
    return NULL;
}

static char *randid(void)
{
    static char buf[9];
    unsigned i;
    time_t t;
    time(&t);
    srand((t >> 8) ^ getpid());
    for (i = 0; i < 9; i++)
        buf[i] = '0' + rand() %10;
    return buf;
}
        
int main(int argc, char *argv[])
{
    char msg[1024];
    int opt;
    struct cmdmsg *cmd;
    unsigned d = 0;
    unsigned v;
    struct device *dev;

    while ((opt = getopt(argc, argv, "d:")) != -1) {
        switch (opt) {
        case 'd':
            d = atoi(optarg);
            break;
        default:
            usage();
        }
    }
    if (optind >= argc)
        usage();
    cmd = find(argv[optind]);
    if (cmd == NULL) {
        fprintf(stderr, "%s: unknown command '%s'.\n", argv[0], argv[optind]);
            exit(EXIT_FAILURE);
    }
    dev = load_device(d);
    if (cmd->flags & FL_WITH_INT) {
        if (optind >= argc)
            usage();
        v = atoi(argv[optind++]);
        snprintf(msg, 1024, cmd->msg, randid(), v);
    } else
        snprintf(msg, 1024, cmd->msg, randid());
    if (optind != argc - 1) {
        fprintf(stderr, "%s: unexpected arguments.\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    post_message(dev, msg);
    return 0;
}
