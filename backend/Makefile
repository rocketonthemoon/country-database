# Backend build + static analysis. Used by the Dockerfile and CI.
#
#   make              hardened release build  -> ./server
#   make sanitize     ASan + UBSan build      -> ./server-asan
#   make analyze      gcc -fanalyzer
#   make cppcheck     cppcheck
#   make tidy         clang-tidy (clang static analyzer + bugprone + cert)
#   make flawfinder   flawfinder (CWE-oriented source scan)
#   make scan         all of the static scans above
#
# Only src/ is held to -Werror and scanned. vendor/mongoose.c is upstream
# code and is compiled with default warnings.

CC      ?= gcc
CFLAGS  ?= -O2
WARN     = -Wall -Wextra -Wformat=2 -Wformat-security -Wshadow \
           -Wstrict-prototypes -Wmissing-prototypes -Wnull-dereference \
           -Wvla -Werror
HARDEN   = -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fstack-clash-protection \
           -fPIE
LDHARDEN = -pie -Wl,-z,relro,-z,now,-z,noexecstack
INCLUDES = -Ivendor
LIBS     = -lsqlite3

SRC      = src/main.c
VENDOR   = vendor/mongoose.c

.PHONY: all sanitize analyze cppcheck tidy flawfinder scan clean

all: server

server: $(SRC) $(VENDOR) vendor/mongoose.h
	$(CC) $(CFLAGS) $(HARDEN) $(INCLUDES) -c $(VENDOR) -o mongoose.o
	$(CC) $(CFLAGS) $(HARDEN) $(WARN) $(INCLUDES) -c $(SRC) -o main.o
	$(CC) $(LDHARDEN) -o $@ main.o mongoose.o $(LIBS)

server-asan: $(SRC) $(VENDOR) vendor/mongoose.h
	$(CC) -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined \
	  -fno-sanitize-recover=undefined $(WARN) -Wno-error $(INCLUDES) \
	  -o $@ $(SRC) $(VENDOR) $(LIBS)

sanitize: server-asan

analyze:
	$(CC) -fanalyzer -O1 $(WARN) $(INCLUDES) -c $(SRC) -o /dev/null

cppcheck:
	cppcheck --std=c11 --enable=warning,style,performance,portability \
	  --error-exitcode=1 --inline-suppr --quiet \
	  --suppress=missingIncludeSystem --suppress=missingInclude \
	  --suppress=checkersReport src/

tidy:
	clang-tidy $(SRC) --quiet --warnings-as-errors='*' \
	  --checks='-*,clang-analyzer-*,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling,bugprone-*,-bugprone-easily-swappable-parameters,cert-*,-cert-err33-c' \
	  -- $(INCLUDES) -std=c11

flawfinder:
	flawfinder --minlevel=3 --error-level=3 --quiet --dataonly src/

scan: analyze cppcheck tidy flawfinder

clean:
	rm -f server server-asan *.o
