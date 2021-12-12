LAYER         = 2
FFAT_OBJ      = src/ffat.o src/layer0.o src/layer1.o src/layer2.o
TEST_OBJ      = tests/main.o tests/image.o tests/scenario.o tests/test.o tests/ff/ff.o tests/tags.o
SAMPLE_OBJ    = sample/sample.o tests/ff/ff.o
CFLAGS        = -std=c11
CPPFLAGS      = -Wall -Wextra
MCU           = atmega16
MAX_CODE_SIZE = 8192
MAX_DATA_SIZE = 640
all: ftest

ftest: CPPFLAGS += -g -O0 -DFFAT_DEBUG=1
ftest: ${FFAT_OBJ} ${TEST_OBJ}
	gcc $^ -o $@
.PHONY: ftest

test: ftest
	./ftest
.PHONY: ftest

size: CC=avr-gcc
size: CPPFLAGS += -Os -mmcu=${MCU} -ffunction-sections -fdata-sections -mcall-prologues
size: ${FFAT_OBJ} tests/size.o
	avr-gcc -mmcu=${MCU} -o $@.elf $^ -Wl,--gc-sections
	avr-size -C --mcu=${MCU} $@.elf


${FFAT_OBJ}:
check-code-size: size
	if [ "`avr-size -B --mcu=atmega16 size.elf | tail -n1 | tr -s ' ' | cut -d ' ' -f 2`" -gt "${MAX_CODE_SIZE}" ]; then \
		>&2 echo "Code is too large.";  \
		false; \
	fi

check-data-size: size
	if [ "`avr-size -B --mcu=atmega16 size.elf | tail -n1 | tr -s ' ' | cut -d ' ' -f 4`" -gt "${MAX_DATA_SIZE}" ]; then \
		>&2 echo "Data usage is too large.";  \
		false; \
	fi

samplex: CPPFLAGS += -g -O0
samplex: ${SAMPLE_OBJ}
	gcc -o samplex $^

sample: samplex
	./samplex | hexdump -C > sample/sample.txt
.PHONY: sample

tests/tags.o: tests/TAGS.TXT
	objcopy -I binary -O pe-x86-64 --binary-architecture i386:x86-64 $^ $@

clean:
	rm -f ${FFAT_OBJ} ${TEST_OBJ} ftest size.elf tests/size.o samplex sample/sample.o
.PHONY: clean