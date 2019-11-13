CC=gcc
SOURCES=ktv.c ktv_test.c
JSON_SOURCES=ktv.c ktv_json_test.c ext/cJSON.c ext/ktv_json.c
PROGRAMS = ktv_test  ktv_json_test

all: ${PROGRAMS}

clean:
	rm -f ${PROGRAMS}

ktv_test: $(SOURCES)
	$(CC) -o ktv_test $(SOURCES) 

ktv_json_test: $(SOURCES)
	$(CC) -o ktv_json_test $(JSON_SOURCES) 
