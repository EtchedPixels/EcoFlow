/*
 *	Monitor an ecoflow device
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>
#include "ecolib.h"

static void usage(void)
{
    fprintf(stderr, "Erm ?\n");
    exit(EXIT_FAILURE);
}

struct jthing {
    const char *name;
    int type;
    long value;
};
#define JINT	1
#define JSTRING	2
#define JFLOAT	3
#define JARRAY	4
#define JSTRUCT	5

static const char *lastname = "";

static void syntax(const char *m, const char *b)
{
    fprintf(stderr, "%s: (%s) \"%s\"\n", lastname, m, b);
    exit(EXIT_FAILURE);
}

struct jthing *token(char **p)
{
    static struct jthing tok;
    char *x = *p;
    long val;
    int sign = 1;

    while(isspace(*x))
        x++;
    if (*x == '}') {	/* end of block */
        *p = x + 1;
        return NULL;
    }
    if (*x != '"')
        syntax("quote expected", x);
    x++;
    tok.name = x++;
    lastname = tok.name;
    while(*x && *x != '"')
        x++;
    if (*x == 0)
        syntax("missing quote", x);
    *x++ = 0;
    while(isspace(*x))
        x++;
    if (*x != ':')
        syntax("colon required", x);
    x++;
    while(isspace(*x))
        x++;
    switch(*x++) {
    case '{':
        /* Sub structure entry */
        tok.type = JSTRUCT;
        *p = x;
        return &tok;
    case '[':	/* Array is used for bmsInsConnt but we don't care */
        while(*x && *x != ']')
            x++;
        if (!*x)
            syntax("close square expected", x);
        x++;
        while(isspace(*x))
            x++;
        if (*x == ',')
            x++;
        tok.type = JARRAY;
        *p = x;
        return &tok;
    case '"':
        while(*x && *x != '"')
            x++;
        if (!*x)
            syntax("quote expected", x);
        x++;
        if (*x == ',')
            x++;
        tok.type = JSTRING;
        *p = x;
        return &tok;
    default:
        /* Number */
        x--;
        tok.type = JINT;
        val = 0;
        if (*x == '-') { 
            sign = -1;
            x++;
        }
        else if (*x == '+') { 
            sign = 1;
            x++;
        }
        while(isdigit(*x)) {
            val *= 10;
            val += *x - '0';
            x++;
        }
        if (*x == '.') {	/* TODO float */
            x++;
            while(isdigit(*x))
                x++;
        }
        while(isspace(*x))
            x++;
        if (*x != '}' && *x != ',')
            syntax("comma or close expected", x);
        if (*x == ',')
            x++;
        *p = x;
        tok.value = val * sign;
        return &tok;
    }
    /* Unreachable */
}
    
void scan_data(const char *p, struct jthing *j)
{
    /* All our properties should be numbers */
    if (j->type != JINT)
        return;    
    if (strcmp(p, j->name) == 0) {
        printf("%ld\n", j->value);
        exit(0);
    }
}

void parse_props(const char *prop, char **buf)
{
    struct jthing *t;
    while((t = token(buf)) != NULL)
           scan_data(prop, t);
}

void sort_of_parse_json(const char *prop, char *buf)
{
    struct jthing *t;
    /* The stuff we need to take apart may be json but we only need
       to do some simple hacks for now */
    while((t = token(&buf)) != NULL) {
        if (strcmp(t->name, "params") == 0)
            parse_props(prop, &buf);
    }
}

void process_mqtt_stream(FILE *f, const char *prop)
{
    char buf[1024];
    while(1) {
        /* We get json objects back of all things */
        if (fgets(buf, 1023, f) == NULL || *buf != '{')
            break;
        sort_of_parse_json(prop, buf + 1);
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    FILE *f;
    int opt;
    unsigned d = 0;
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
    /* rest of args - error for now */
    
    if (optind != argc - 1) {
        usage();
    }

    dev = load_device(d);
    f = attach_mqtt_stream(dev);
    while(1) {
          process_mqtt_stream(f, argv[optind]);
    }
}
