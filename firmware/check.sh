#!/bin/bash
# Sanity check for firmware structure and completeness

set -e

echo "🔍 Spooldex Firmware Sanity Check"
echo "=================================="

# Check required files exist
echo -n "✓ Checking required files... "
files=(
    "CMakeLists.txt"
    "main/CMakeLists.txt"
    "main/main.c"
    "main/Kconfig"
    "sdkconfig.defaults"
    "partitions.csv"
    "README.md"
)

for file in "${files[@]}"; do
    if [ ! -f "$file" ]; then
        echo "❌ Missing: $file"
        exit 1
    fi
done
echo "OK"

# Check all source files have headers
echo -n "✓ Checking source/header pairs... "
for c_file in main/*.c; do
    base=$(basename "$c_file" .c)
    h_file="main/${base}.h"
    
    # main.c doesn't need a header, mqtt_client.c has mqtt_publisher.h
    if [ "$base" = "main" ] || [ "$base" = "mqtt_client" ]; then
        continue
    fi
    
    if [ ! -f "$h_file" ]; then
        echo "⚠️  Warning: $c_file has no matching header"
    fi
done
echo "OK"

# Check for common mistakes
echo -n "✓ Checking for common issues... "

# Check that mqtt_client.c uses mqtt_publisher.h
if ! grep -q "#include \"mqtt_publisher.h\"" main/mqtt_client.c; then
    echo "⚠️  mqtt_client.c should include mqtt_publisher.h"
fi

# Check for TODO markers
todo_count=$(grep -r "TODO" main/ --include="*.c" --include="*.h" | wc -l | tr -d ' ')
if [ "$todo_count" -gt 0 ]; then
    echo "ℹ️  Found $todo_count TODO items (this is OK)"
fi

echo "OK"

# Count lines of code
echo ""
echo "📊 Statistics:"
c_files=$(find main -name "*.c" | wc -l | tr -d ' ')
h_files=$(find main -name "*.h" | wc -l | tr -d ' ')
total_loc=$(find main -name "*.c" -o -name "*.h" | xargs wc -l | tail -1 | awk '{print $1}')

echo "   Source files: $c_files"
echo "   Header files: $h_files"
echo "   Total LOC: $total_loc"

# List modules
echo ""
echo "📦 Modules:"
for h_file in main/*.h; do
    base=$(basename "$h_file" .h)
    if [ -f "main/${base}.c" ]; then
        echo "   - $base"
    fi
done

echo ""
echo "✅ All checks passed!"
echo ""
echo "Next steps:"
echo "  1. idf.py set-target esp32c6"
echo "  2. idf.py build"
echo "  3. idf.py -p PORT flash monitor"
