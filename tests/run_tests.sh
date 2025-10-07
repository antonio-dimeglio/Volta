#!/bin/bash
# Volta Test Runner
# Usage: ./run_tests.sh [category]
# Categories: all, arrays, loops, basics, edge_cases

set -e

VOLTA_BIN="./bin/volta"
TEST_DIR="tests/volta_programs"
PASSED=0
FAILED=0

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

run_test() {
    local test_file=$1
    local test_name=$(basename "$test_file" .volta)
    
    echo -n "Running $test_name... "
    
    if timeout 5 $VOLTA_BIN "$test_file" > /tmp/volta_test_out.txt 2>&1; then
        echo -e "${GREEN}PASSED${NC}"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}FAILED${NC}"
        echo "  Output:"
        cat /tmp/volta_test_out.txt | head -20 | sed 's/^/    /'
        ((FAILED++))
        return 1
    fi
}

run_category() {
    local category=$1
    local cat_dir="$TEST_DIR/$category"
    
    if [ ! -d "$cat_dir" ]; then
        echo -e "${YELLOW}Warning: Category '$category' not found${NC}"
        return
    fi
    
    echo ""
    echo "========================================"
    echo "  Testing: $category"
    echo "========================================"
    
    for test_file in "$cat_dir"/*.volta; do
        if [ -f "$test_file" ]; then
            run_test "$test_file"
        fi
    done
}

# Check if volta binary exists
if [ ! -f "$VOLTA_BIN" ]; then
    echo -e "${RED}Error: Volta binary not found at $VOLTA_BIN${NC}"
    echo "Please run 'make' first"
    exit 1
fi

# Parse arguments
CATEGORY="${1:-all}"

case "$CATEGORY" in
    all)
        echo "Running ALL tests..."
        run_category "basics"
        run_category "arrays"
        run_category "loops"
        run_category "edge_cases"
        ;;
    arrays|loops|basics|edge_cases)
        run_category "$CATEGORY"
        ;;
    *)
        echo "Unknown category: $CATEGORY"
        echo "Available categories: all, arrays, loops, basics, edge_cases"
        exit 1
        ;;
esac

# Summary
echo ""
echo "========================================"
echo "  Test Summary"
echo "========================================"
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo "Total:  $((PASSED + FAILED))"

if [ $FAILED -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed${NC}"
    exit 1
fi
