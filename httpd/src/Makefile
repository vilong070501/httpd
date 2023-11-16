include flags.mk

OBJ = main.o

LIBS = \
	   config/libconfig.a \
	   utils/libutils.a \
	   daemon/libdaemon.a \
	   server/libserver.a \

SUBDIRS = \
		  config \
		  utils \
		  daemon \
		  server \

all: httpd

httpd: $(OBJ) $(LIBS)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) -Wl,--start-group $(LIBS) -Wl,--end-group

clean:
	for I in $(SUBDIRS);			\
	do					\
		$(MAKE) -C $$I $@ || exit 1;	\
	done
	$(RM) main.o

include libs.mk

.PHONY: all
