FFAT_OBJ      = src/ffat.o
TEST_OBJ      = tests/main.o
CFLAGS        = -std=c11
CPPFLAGS      = -Wall -Wextra
MCU           = atmega16
MAX_CODE_SIZE = 8192
MAX_DATA_SIZE = 640
all: ftest

ftest: CPPFLAGS += -g -O0 -DFFAT_DEBUG=1
ftest: ${FFAT_OBJ} ${TEST_OBJ}
	g++ $^ -o $@
.PHONY: ftest

test: ftest
	./ftest
.PHONY: ftest

clean:
	rm -f ${FFAT_OBJ} ${TEST_OBJ} ftest size.elf size/size.o
.PHONY: clean