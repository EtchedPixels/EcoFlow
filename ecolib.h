struct device {
    char *user;
    char *pass;
    char *id;
    char *ident;
    char *serial;
};

extern char *xstrdup(const char *p);
extern struct device *load_device(unsigned n);

FILE *attach_mqtt_stream(struct device *dev);

