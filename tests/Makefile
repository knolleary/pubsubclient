SRC_PATH=./src
OUT_PATH=./bin
TEST_SRC=$(wildcard ${SRC_PATH}/*_spec.cpp)
TEST_BIN= $(TEST_SRC:${SRC_PATH}/%.cpp=${OUT_PATH}/%)
VPATH=${SRC_PATH}
SHIM_FILES=${SRC_PATH}/lib/*.cpp
PSC_FILE=../PubSubClient/PubSubClient.cpp
CC=g++
CFLAGS=-I${SRC_PATH}/lib -I../PubSubClient

all: $(TEST_BIN)

${OUT_PATH}/%: ${SRC_PATH}/%.cpp ${PSC_FILE} ${SHIM_FILES}
	mkdir -p ${OUT_PATH}
	${CC} ${CFLAGS} $^ -o $@
	
clean:
	@rm -rf ${OUT_PATH}
