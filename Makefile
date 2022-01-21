CFLAGS := -std=gnu18 -g -Wall -Wextra -pedantic -Os -flto
LDFLAGS := -Wl,--gc-sections
target := tex2dds

ifeq ($(OS),Windows_NT)
    target := $(target).exe
endif

all: $(target)
strip: LDFLAGS := $(LDFLAGS) -s
strip: all

object_files = main.o

main.o: dds.h tex.h

$(target): $(object_files)
	$(LINK.c) $^ -o $@


clean:
	rm -f $(target) $(object_files)
