# AXON - Encyclopedia Filesystem for TaijiOS
<../mkfile.common

OBJTYPE=amd64
O=$OBJTYPE

# Directories
SRC=lib
INCLUDE=include
BUILD=build
BIN=bin
DATA=data

# Toolchain - Use Plan 9 compilers from sys
CC=9c
LD=9l
AR=9ar

# Paths
ROOT=..
SYS=$ROOT/sys
MARROW=$ROOT/marrow
LIB9_INCLUDE=$SYS/include
LIB9_LIB=$ROOT/amd64/lib/lib9.a

# Flags
CFLAGS=-FTVwp -I$INCLUDE -I$LIB9_INCLUDE
LDFLAGS=-L$ROOT/amd64/lib -l9 -lpthread

# Target
LIB=$BUILD/libaxon.a
TARGET=$BIN/axon

# Source files by module
AXON_CORE=axon
AXON_ENCYCLOPEDIA=encyclopedia/entry encyclopedia/fact encyclopedia/citation \
                  encyclopedia/search encyclopedia/index encyclopedia/consensus
AXON_FS=fs/tree fs/ops fs/handlers fs/files fs/ctl
AXON_INGEST=ingest/parser ingest/extractor ingest/pipeline
AXON_MEMORY=memory/episodic memory/semantic memory/procedural

# All objects
OFILES=${AXON_CORE:%=$BUILD/%.$O} \
       ${AXON_ENCYCLOPEDIA:%=$BUILD/%.$O} \
       ${AXON_FS:%=$BUILD/%.$O} \
       ${AXON_INGEST:%=$BUILD/%.$O} \
       ${AXON_MEMORY:%=$BUILD/%.$O}

# Default target
all:V: setup $LIB $TARGET

setup:V:
	mkdir -p $BUILD/encyclopedia $BUILD/fs $BUILD/ingest $BUILD/memory
	mkdir -p $BIN $DATA/encyclopedia/{entries,confidence,contradictions,sources}
	mkdir -p $DATA/index

# Library
$LIB: $OFILES $LIB9_LIB
	ar rvc $target $OFILES

# Server binary
$TARGET: cmd/axon/main.c $LIB $LIB9_LIB
	$LD $CFLAGS -I$INCLUDE -I$SRC -I$LIB9_INCLUDE cmd/axon/main.c -L$BUILD -laxon -l9 -o $target $LDFLAGS

# Compile rules
$BUILD/%.$O: $SRC/%.c
	$CC $CFLAGS -I$INCLUDE -I$SRC -c $prereq -o $target

$BUILD/encyclopedia/%.$O: $SRC/encyclopedia/%.c
	$CC $CFLAGS -I$INCLUDE -I$SRC -c $prereq -o $target

$BUILD/fs/%.$O: $SRC/fs/%.c
	$CC $CFLAGS -I$INCLUDE -I$SRC -c $prereq -o $target

$BUILD/ingest/%.$O: $SRC/ingest/%.c
	$CC $CFLAGS -I$INCLUDE -I$SRC -c $prereq -o $target

$BUILD/memory/%.$O: $SRC/memory/%.c
	$CC $CFLAGS -I$INCLUDE -I$SRC -c $prereq -o $target

clean:V:
	rm -rf $BUILD $BIN

install:V: $TARGET
	cp $TARGET $ROOT/bin/axon
	chmod +x $ROOT/bin/axon
