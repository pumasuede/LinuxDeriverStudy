make

./clean.sh > /dev/null 2>&1

MODULE="raxiao"
DEVICE="sbull"

insmod $MODULE.ko $* || exit 1
major=`cat /proc/devices | grep $DEVICE | head -n 1| cut -d' ' -f1`
echo "major:" $major

#rm -f /dev/${DEVICE}[a-d][0-3]
#mknod /dev/${DEVICE}a0 b $major 0
#chmod 666 /dev/${DEVICE}a0
