#!/bin/bash
# Debug script for WeedlyWeb

echo "🔍 Debugging WeedlyWeb..."
echo ""

# Check dependencies
echo "📦 Checking dependencies:"
pkg-config --modversion webkit2gtk-4.1 2>/dev/null && echo "✅ WebKit2GTK found" || echo "❌ WebKit2GTK not found"
pkg-config --modversion gtk+-3.0 2>/dev/null && echo "✅ GTK+3 found" || echo "❌ GTK+3 not found"
echo ""

# Check linked libraries
echo "📚 Linked WebKit libraries:"
ldd build/WeedlyWeb 2>/dev/null | grep webkit || echo "❌ No WebKit library found"
echo ""

# Enable GTK debug messages
export G_MESSAGES_DEBUG=all
export WEBKIT_DISABLE_COMPOSITING_MODE=1

# Launch with strace to see system calls (if available)
if command -v strace >/dev/null 2>&1; then
    echo "🔍 Launching with strace (last 20 lines):"
    timeout 3 strace -e trace=open,openat,mmap,munmap ./build/WeedlyWeb 2>&1 | tail -20
else
    echo "⚠️  strace not available, normal launch with debug messages:"
    timeout 3 ./build/WeedlyWeb 2>&1 | head -30
fi

echo ""
echo "✅ Debug completed"
