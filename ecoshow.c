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

static char *serial;
static char *ident;
static char *user;
static char *pass;
static char *id;

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

static void syntax(const char *m, const char *b)
{
    fprintf(stderr, "%s: \"%s\"\n", m, b);
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
    
struct prop {
    const char *name;
    long value;
};

struct prop river2_props[] = {
    { "pd.usb1Watts", 0 },		/* 0 */
    { "pd.qcUsb1Watts", 0 },
    { "pd.wattsInSum", 0 },
    { "pd.qcUsb2Watts", 0 },
    { "pd.ext3p8Port", 0 },
    { "pd.lcdOffSec", 0 },		/* 5 */
    { "pd.standbyMin", 0 },
    { "pd.extRj45Port", 0 },
    { "pd.beepMode", 0 },
    { "pd.remainTime", 0 },
    { "pd.typec2Watts", 0 },		/* 10 */
    { "pd.carWatts", 0 },
    { "pd.ext4p8Port", 0 },
    { "pd.brightLevel", 0 },
    { "pd.carUsedTime", 0 },
    { "pd.chgDsgState", 0 },		/* 15 */
    { "pd.typec1Watts", 0 },
    { "pd.usb2Watts", 0 },
    { "pd.dcOutState", 0 },
    { "pd.soc", 0 },
    { "pd.carState", 0 },		/* 20 */
    { "pd.invUsedTime", 0 },
    { "pd.wattsOutSum", 0 },

    { "inv.fanState", 0 },
    { "inv.cfgAcEnabled", 0 },
    { "inv.cfgAcXboost", 0 },		/* 25 */
    { "inv.cfgAcWorkMode", 0 },
    { "inv.invType", 0 },
    { "inv.SlowChgWatts", 0 },
    { "inv.acDipSwitch", 0 },
    { "inv.acInVol", 0 },		/* 30 */
    { "inv.FastChgWatts", 0 },
    { "inv.inputWatts", 0 },
    { "inv.outputWatts", 0 },
    { "inv.errCode", 0 },
    { "inv.standbyMins", 0 },		/* 35 */
    { "inv.chgPauseFlag", 0 },
    { "inv.dischargeType", 0 },
    { "inv.chargerType", 0 },
    { "inv.sysVer", 0 },

    { "bms_bmsStatus.errCode", 0 },    	/* 40 */
    { "bms_bmsStatus.outputWatts", 0 },    
    { "bms_bmsStatus.inputWatts", 0 },    
    { "bms_bmsStatus.type", 0 },    
    { "bms_bmsStatus.sysVer", 0 },    
    { "bms_bmsStatus.openBmsIdx", 0 },	/* 45 */
    { "bms_bmsStatus.num", 0 },    
    { "bms_bmsStatus.remainCap", 0 },    
    { "bms_bmsStatus.remainTime", 0 },    
    { "bms_bmsStatus.f32ShowSoc", 0 },    
    { "bms_bmsStatus.tagChgAmp", 0 },    /* 50 */
    { "bms_bmsStatus.bmsFault", 0 },
    { "bms_bmsStatus.soc", 0 },

    { "inv.invOutVol", 0 },
    { "bms.minCellVol", 0 },
    { "bms.maxCellVol", 0 },

    { NULL, 0 }
};

void insert_data(struct prop *p, struct jthing *j)
{
    /* All our properties should be numbers */
    if (j->type != JINT)
        return;    
    while(p->name) {
        if (strcmp(p->name, j->name) == 0) { 
            p->value = j->value;
            return;
        }
        p++;
    }
    printf("unknown param '%s'\n", j->name);
}

void parse_props(char **buf)
{
    struct jthing *t;
    while((t = token(buf)) != NULL) {
        insert_data(river2_props, t);
    }
}

void sort_of_parse_json(char *buf)
{
    struct jthing *t;
    /* The stuff we need to take apart may be json but we only need
       to do some simple hacks for now */
    while((t = token(&buf)) != NULL) {
        if (strcmp(t->name, "params") == 0)
            parse_props(&buf);
    }
}

void process_mqtt_stream(FILE *f)
{
    char buf[1024];
    while(1) {
        /* We get json objects back of all things */
        if (fgets(buf, 1023, f) == NULL || *buf != '{')
            break;
        sort_of_parse_json(buf + 1);
        printf("Inverter Watts In %ldW Watts Out %ldW           \r",
            river2_props[32].value,
            river2_props[33].value);
        fflush(stdout);
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
    if (optind < argc) {
        usage();
    }

    dev = load_device(d);
    f = attach_mqtt_stream(dev);
    while(1) {
          process_mqtt_stream(f);
    }
}
