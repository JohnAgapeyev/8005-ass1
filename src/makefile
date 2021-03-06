CC=gcc
BASEFLAGS=-Wall -Wextra -std=c11 -pedantic -D_POSIX_C_SOURCE=200809L -fopenmp
DEBUGFLAGS=-ggdb -O0
RELEASEFLAGS=-O3 -march=native -flto -DNDEBUG
CLIBS=-pthread -lrt -lcrypto
EXEC=8005-ass1.elf
DEPS=$(EXEC).d
SRCWILD=$(wildcard *.c)
HEADWILD=$(wildcard *.h)

all release debug: $(patsubst %.c, %.o, $(SRCWILD))
	$(CC) $(CFLAGS) $^ $(CLIBS) -o $(EXEC)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $(patsubst %.c, %.o, $<)

$(DEPS): $(SRCWILD) $(HEADWILD)
	@$(CC) $(CFLAGS) -MM $(SRCWILD) > $(DEPS)

ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS)
endif

ifeq (,$(filter debug, $(MAKECMDGOALS)))
$(eval CFLAGS := $(BASEFLAGS) $(RELEASEFLAGS))
else
$(eval CFLAGS := $(BASEFLAGS) $(DEBUGFLAGS))
endif

.PHONY: clean

clean:
	$(RM) $(EXEC) $(wildcard *.o) $(wildcard *.d)
