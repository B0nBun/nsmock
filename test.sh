#!/usr/bin/bash
# This are some very basic tests which try to set/increment/decrement some stats
# and then assert that the output of `ifconfig` command is as expected

set -euo pipefail

NSMOCK_SYSDIR="/sys/kernel/nsmock"

LSMOD=$(lsmod)
if ! grep -q nsmock <<< "$LSMOD"; then
    echo "Expected nsmock module to be installed"
    exit 1
fi

assert_packet_count() {
    PREFIX=$1
    EXPECT=$2
    COUNT=$(ifconfig nsmock0 | grep -Eo "$PREFIX ([0-9]+)" | grep -Eo "[0-9]+")
    if [ $COUNT != $EXPECT ]; then
        echo "Expected $PREFIX packet count to be equal to $EXPECT, but got $COUNT"
        exit 1
    fi
}

STATS=(rx_errors tx_errors rx_packets tx_packets)
IFCONFIG_PREFIXES=("RX errors" "TX errors" "RX packets" "TX packets")

for i in "${!STATS[@]}"; do
    STAT=${STATS[$i]}
    COUNT=$((i + 1))
    echo "storing $COUNT into $STAT"
    echo $COUNT > "$NSMOCK_SYSDIR/$STAT"
    assert_packet_count "${IFCONFIG_PREFIXES[$i]}" $COUNT
done

for i in "${!STATS[@]}"; do
    STAT=${STATS[$i]}
    echo "decrementing $STAT"
    echo "-1" > "$NSMOCK_SYSDIR/$STAT"
    assert_packet_count "${IFCONFIG_PREFIXES[$i]}" $i
done

for i in "${!STATS[@]}"; do
    STAT=${STATS[$i]}
    echo "incrementing $STAT by 10"
    echo "+10" > "$NSMOCK_SYSDIR/$STAT"
    assert_packet_count "${IFCONFIG_PREFIXES[$i]}" $(($i + 10))
done

echo "Tests passed!"