#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <getopt.h>
#include <pwd.h>

#include "ecolib.h"

char *xstrdup(const char *p)
{
    char *v = strdup(p);
    if (v == NULL) {
        fprintf(stderr, "out of memory.\n");
        exit(EXIT_FAILURE);
    }
    return v;
}

char *gethomedir(void)
{
    struct passwd *p = getpwuid(geteuid());
    if (p == NULL) {
        fprintf(stderr, "no home directory ?\n");
        exit(EXIT_FAILURE);
    }
    return p->pw_dir;
}

struct device *load_device(unsigned d)
{
    static struct device dev;
    char *p;
    char *o, *v;
    char buf[512];
    char ser[33];
    int n;
    FILE *fp;

    p = getenv("HOME");
    if (p == NULL)
        p = gethomedir();

    snprintf(buf, sizeof(buf), "%s/.ecoflow", p);

    fp = fopen(buf, "r");
    if (fp == NULL) {
        perror(".ecoflow");
        exit(EXIT_FAILURE);
    }

    while(fgets(buf, 511, fp) != NULL) {
        if (sscanf(buf, "%d: %32s", &n, ser) == 2) {
            if (n == d)
                dev.serial = xstrdup(ser);
            continue;
        }
        o = strtok(buf, " \t");
        v = strtok(NULL, " \t\n");
        if (o == NULL || v == NULL)
            continue;
        v = xstrdup(v);
        if (strcmp(o, "ident") == 0)
            dev.ident = v;
        else if (strcmp(o, "userid") == 0)
            dev.user = v;
        else if (strcmp(o, "password") == 0)
            dev.pass = v;
        else if (strcmp(o, "id") == 0)
            dev.id = v;
        else {
            fprintf(stderr, "uinknown property '%s'\n", v);
            exit(EXIT_FAILURE);
        }
    }
    if (dev.serial == NULL) {
        fprintf(stderr, "No configuration for device %d\n", d);
        exit(EXIT_FAILURE);
    }
    if (dev.id == NULL || dev.ident == NULL || dev.user == NULL || dev.pass == NULL) {
        fprintf(stderr, "Authentication data missing\n");
        exit(EXIT_FAILURE);
    }
    return &dev;
}

/* This needs to turn into routines using mosquitto as a library */
FILE *attach_mqtt_stream(struct device *dev)
{
    FILE *f;
    char buf[1024];

    sprintf(buf, "mosquitto_sub -h mqtt-e.ecoflow.com -p 8883 "
    "-u \"app-%s\" -P \"%s\" -i \"%s_%s\" -t \"/app/device/property/%s\"",
        dev->user, dev->pass, dev->ident, dev->id, dev->serial);
    printf("%s\n", buf);
    f = popen(buf, "r");
    if (f == NULL) {
        perror("msquitto_sub");
        exit(EXIT_FAILURE);
    }
    return f;
}

