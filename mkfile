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
INGEST=$BIN/axoningest

# Source files by module
AXON_CORE=axon
AXON_ENCYCLOPEDIA=encyclopedia/entry encyclopedia/fact encyclopedia/citation \
                  encyclopedia/search encyclopedia/index encyclopedia/consensus
AXON_FS=fs/tree fs/ops fs/handlers fs/files fs/ctl
AXON_INGEST=ingest/parser ingest/extractor ingest/pipeline ingest/mind_runner
AXON_MEMORY=memory/episodic memory/semantic memory/procedural
AXON_MINDS=minds/mind minds/literal minds/skeptic minds/synthesizer \
           minds/pattern_matcher minds/questioner
AXON_FACTS=facts/store facts/query

# All objects
OFILES=${AXON_CORE:%=$BUILD/%.$O} \
       ${AXON_ENCYCLOPEDIA:%=$BUILD/%.$O} \
       ${AXON_FS:%=$BUILD/%.$O} \
       ${AXON_INGEST:%=$BUILD/%.$O} \
       ${AXON_MEMORY:%=$BUILD/%.$O} \
       ${AXON_MINDS:%=$BUILD/%.$O} \
       ${AXON_FACTS:%=$BUILD/%.$O}

# Default target
all:V: setup $LIB $TARGET $INGEST

test:V: $TEST
	$TEST

# Compile rules

setup:V:
	mkdir -p $BUILD/encyclopedia $BUILD/fs $BUILD/ingest $BUILD/memory $BUILD/minds $BUILD/facts
	mkdir -p $BIN

	# Knowledge directory (immutable entries - Omnia Codex hierarchy)
	mkdir -p $DATA/knowledge/01_Cosmos_and_Nature/Physics_and_Acoustics
	mkdir -p $DATA/knowledge/01_Cosmos_and_Nature/Astronomy_and_Cosmology
	mkdir -p $DATA/knowledge/02_Humanity_and_Civilization
	mkdir -p $DATA/knowledge/03_Philosophy_and_Consciousness
	mkdir -p $DATA/knowledge/04_Technology_and_Systems

	# Minds directory (AI extraction agents)
	mkdir -p $DATA/minds/literal/facts
	mkdir -p $DATA/minds/skeptic/facts
	mkdir -p $DATA/minds/synthesizer/facts
	mkdir -p $DATA/minds/pattern_matcher/facts
	mkdir -p $DATA/minds/questioner/facts

	# Facts directory (extracted, mutable facts)
	mkdir -p $DATA/facts/by_subject
	mkdir -p $DATA/facts/by_predicate
	mkdir -p $DATA/facts/consensus

	# Contradictions directory (conflicts to resolve)
	mkdir -p $DATA/contradictions

	# Inbox directory (new knowledge to process)
	mkdir -p $DATA/inbox/incoming

	# Index directory
	mkdir -p $DATA/index

# Library
$LIB: $OFILES $LIB9_LIB
	ar rvc $target $OFILES

# Server binary
$TARGET: cmd/axon/main.c $LIB $LIB9_LIB
	$LD $CFLAGS -I$INCLUDE -I$SRC -I$LIB9_INCLUDE cmd/axon/main.c -L$BUILD -laxon -l9 -o $target $LDFLAGS

# Ingest tool binary
$INGEST: cmd/axoningest/main.c $LIB $LIB9_LIB
	$LD $CFLAGS -I$LIB9_INCLUDE cmd/axoningest/main.c -l9 -o $target $LDFLAGS

# Test binary
TEST=$BIN/mind_test
$TEST: tests/mind_test.c $LIB $LIB9_LIB
	$LD $CFLAGS -I$INCLUDE -I$SRC -I$LIB9_INCLUDE tests/mind_test.c -L$BUILD -laxon -l9 -o $target $LDFLAGS

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

$BUILD/minds/%.$O: $SRC/minds/%.c
	$CC $CFLAGS -I$INCLUDE -I$SRC -c $prereq -o $target

$BUILD/facts/%.$O: $SRC/facts/%.c
	$CC $CFLAGS -I$INCLUDE -I$SRC -c $prereq -o $target

clean:V:
	rm -rf $BUILD $BIN

install:V: $TARGET $INGEST
	cp $TARGET $ROOT/bin/axon
	cp $INGEST $ROOT/bin/axoningest
	chmod +x $ROOT/bin/axon $ROOT/bin/axoningest
