export CC = gcc
export CPPFLAGS = -Isrc
export CFLAGS = -std=c99 -Werror -Wall -Wextra -Wvla -pedantic

BINARY=httpd

all: $(BINARY)

$(BINARY):
	$(MAKE) -C src
	mv src/$(BINARY) .

clean:
	$(MAKE) -C src $@
	$(RM) $(BINARY)

check:
	$(MAKE) -C src $@

.PHONY: all
