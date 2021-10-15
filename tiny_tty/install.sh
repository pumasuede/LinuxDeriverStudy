make

./clean.sh > /dev/null 2>&1

MODULE="tiny_tty"
DEVICE="ttty"

insmod $MODULE.ko $* || exit 1
major=`cat /proc/tty/drivers | grep $MODULE | head -n 1| awk '{print $3}'`
echo "major:" $major

chmod 777 /dev/${DEVICE}0
#rm -f /dev/${DEVICE}[a-d][0-3]
#mknod /dev/${DEVICE}a0 b $major 0
