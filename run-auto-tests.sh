#!/bin/bash

SHOW_STDERR=${SHOW_STDERR:-0}
USE_VALGRIND=${USE_VALGRIND:-0}
VALGRIND="valgrind --leak-check=full --num-callers=100 --error-exitcode=1"
VALGRIND="${VALGRIND} --exit-on-first-error=yes"
#VALGRIND="${VALGRIND} --suppressions=/usr/lib/x86_64-linux-gnu/valgrind/default.supp"
VALGRIND="${VALGRIND} --suppressions=/usr/share/glib-2.0/valgrind/glib.supp"
VALGRIND="${VALGRIND} --suppressions=Source/valgrind/valgrind.supp"

start_time=$(date +%s)
total_passed=0
total_failed=0
total_crashed=0
test_failed=""
test_crashed=""

export MG_GAL_ENGINE=dummy
export MG_IAL_ENGINE=dummy

(cd 4.0; ./fetch-ucd-test.sh)
(cd src; ./mginit &)
sleep 3

truncate -s 0 /var/tmp/minigui-tests.log

for subdir in api 4.0 5.0; do
    cd $subdir

    TEST_PROGS=`find . -perm -0111 -type f`

    for x in $TEST_PROGS; do
        echo ">> Start of $subdir/$x"
        if test $USE_VALGRIND -eq 0; then
            if test $SHOW_STDERR -eq 0; then
                echo ">> STDERR OF $subdir/$x" >> /var/tmp/minigui-tests.log
                timeout 10m $x auto 2>> /var/tmp/minigui-tests.log
                RESULT=$?
                echo "<< END OF STDERR OF $subdir/$x" >> /var/tmp/minigui-tests.log
                echo "" >> /var/tmp/minigui-tests.log
            else
                timeout 10m $x auto
                RESULT=$?
            fi
        else
            ${VALGRIND} ./$x auto || exit
            RESULT=$?
        fi

        if test $RESULT -eq 0; then
            total_passed=$((total_passed + 1))
        elif test $RESULT -gt 128; then
            total_crashed=$((total_crashed + 1))
            test_crashed="$subdir/$x $test_crashed"
        else
            total_failed=$((total_failed + 1))
            test_failed="$subdir/$x $test_failed"
        fi
        echo "<< End of $subdir/$x"
        echo ""
    done

    cd ..
done

total=$((total_passed + total_failed + total_crashed))

end_time=$(date +%s)
time=$(($end_time - $start_time))

echo "#######"
echo "# Tests run:      $total (total time $time second(s))"
echo "# Passed:         $total_passed"
echo "# Failed:         $total_failed"
echo "# Crashed:        $total_crashed"
echo "#######"

if test $total_failed -ne 0; then
    echo "Failed tests:"
    for x in $test_failed; do
        echo $x
    done
fi

if test $total_crashed -ne 0; then
    echo "Crashed tests:"
    for x in $test_crashed; do
        echo $x
    done
fi

killall mginit

total_not_passed=$((total_failed + total_crashed))
exit $total_not_passed

