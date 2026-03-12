#!/bin/sh

# AXON Basic Usage Example
# This script demonstrates basic AXON functionality

AXON_PORT=17020
AXON_DATA="./axon/data"
MOUNT_POINT="/mnt/axon"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "${BLUE}AXON Basic Usage Example${NC}"
echo "=============================="
echo

# Check if axon binary exists
if [ ! -f "./bin/axon" ]; then
    echo "Error: axon binary not found. Please build first:"
    echo "  mk axon-build"
    exit 1
fi

echo "${GREEN}1. Starting AXON server...${NC}"
./bin/axon -p $AXON_PORT -d $AXON_DATA &
AXON_PID=$!
sleep 2

echo "${GREEN}2. Creating mount point...${NC}"
mkdir -p $MOUNT_POINT

echo "${GREEN}3. Mounting AXON filesystem...${NC}"
echo "(This requires a 9p client - for now, we'll simulate operations)"
echo

# Simulate control commands via direct file operations
echo "${GREEN}4. Adding sample entries...${NC}"

# Create sample entry files
mkdir -p $AXON_DATA/encyclopedia/entries

cat > $AXON_DATA/encyclopedia/entries/Eiffel_Tower << 'EOF'
# Eiffel Tower

Confidence: 0.95

The Eiffel Tower is a wrought-iron lattice tower on the Champ de Mars in Paris,
France. It is named after the engineer Gustave Eiffel, whose company designed
and built the tower.

Locally nicknamed "La dame de fer" (French for "Iron Lady"), it was constructed
from 1887 to 1889 as the entrance to the 1889 World's Fair.
EOF

cat > $AXON_DATA/encyclopedia/entries/Python << 'EOF'
# Python

Confidence: 0.90

Python is a high-level, general-purpose programming language. Its design philosophy
emphasizes code readability with the use of significant indentation.

Python is dynamically typed and garbage-collected. It supports multiple programming
paradigms, including structured, object-oriented and functional programming.
EOF

cat > $AXON_DATA/encyclopedia/entries/Plan_9 << 'EOF'
# Plan 9

Confidence: 0.95

Plan 9 from Bell Labs is a distributed operating system. It was developed primarily
for research purposes as the successor to UNIX by the Computing Sciences Research
Center at Bell Labs between the mid-1980s and 2002.

Plan 9 takes the "everything is a file" concept further than UNIX, representing
all system resources as files.
EOF

echo "Created 3 sample entries."
echo

echo "${GREEN}5. Sample operations (when mounted):${NC}"
echo
echo "  # Read status"
echo "  cat $MOUNT_POINT/status"
echo
echo "  # Search for 'tower'"
echo "  echo 'tower' > $MOUNT_POINT/encyclopedia/search/query"
echo "  cat $MOUNT_POINT/encyclopedia/search/results"
echo
echo "  # Read an entry"
echo "  cat $MOUNT_POINT/encyclopedia/entries/Eiffel_Tower"
echo
echo "  # Add a new entry via control file"
echo "  echo 'add_entry TaijiOS TaijiOS is a Plan 9 inspired OS...' > $MOUNT_POINT/ctl"
echo
echo "  # Search using standard tools"
echo "  grep -r 'language' $MOUNT_POINT/encyclopedia/entries/"
echo

echo "${GREEN}6. Current data directory contents:${NC}"
ls -la $AXON_DATA/encyclopedia/entries/
echo

echo "${GREEN}Stopping AXON server...${NC}"
kill $AXON_PID 2>/dev/null
wait $AXON_PID 2>/dev/null

echo "${BLUE}Example complete!${NC}"
echo
echo "Note: To fully interact with the 9P filesystem, you need a 9p client."
echo "The data files have been created in $AXON_DATA for inspection."
