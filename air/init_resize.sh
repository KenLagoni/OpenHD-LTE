#!/bin/sh

fix_partuuid() {
  mount -o remount,rw "$ROOT_PART_DEV"
  mount -o remount,rw "$BOOT_PART_DEV"
  DISKID="$(tr -dc 'a-f0-9' < /dev/hwrng | dd bs=1 count=8 2>/dev/null)"
  fdisk "$ROOT_DEV" > /dev/null <<EOF
x
i
0x$DISKID
r
w
EOF
  if [ "$?" -eq 0 ]; then
    sed -i "s/${OLD_DISKID}/${DISKID}/g" /etc/fstab
    sed -i "s/${OLD_DISKID}/${DISKID}/" /boot/cmdline.txt
    sync
  fi

  mount -o remount,ro "$ROOT_PART_DEV"
  mount -o remount,ro "$BOOT_PART_DEV"
}

# 8GB Before resize:
#
# Command (m for help): p
# Disk /dev/mmcblk0: 7.4 GiB, 7948206080 bytes, 15523840 sectors
# Units: sectors of 1 * 512 = 512 bytes
# Sector size (logical/physical): 512 bytes / 512 bytes
# I/O size (minimum/optimal): 512 bytes / 512 bytes
# Disklabel type: dos
# Disk identifier: 0x9730496b

# Device         Boot  Start     End Sectors  Size Id Type
# /dev/mmcblk0p1        8192  532479  524288  256M  c W95 FAT32 (LBA)
# /dev/mmcblk0p2      532480 3661823 3129344  1.5G 83 Linux



# 8GB after resize:
# command (m for help): p
# Disk /dev/mmcblk0: 7.4 GiB, 7948206080 bytes, 15523840 sectors
# Units: sectors of 1 * 512 = 512 bytes
# Sector size (logical/physical): 512 bytes / 512 bytes
# I/O size (minimum/optimal): 512 bytes / 512 bytes
# Disklabel type: dos
# Disk identifier: 0x6ef2fd02
#
# Device         Boot   Start      End  Sectors  Size Id Type
# /dev/mmcblk0p1         8192   532479   524288  256M  c W95 FAT32 (LBA)
# /dev/mmcblk0p2       532480  3907583  3375104  1.6G 83 Linux
# /dev/mmcblk0p3      3907584 15523839 11616256  5.6G  c W95 FAT32 (LBA)

  

get_variables () {
  ROOT_PART_DEV=$(findmnt / -o source -n)  #/dev/mmcblk0p2
  ROOT_PART_NAME=$(echo "$ROOT_PART_DEV" | cut -d "/" -f 3) #mmcblk0p2
  ROOT_DEV_NAME=$(echo /sys/block/*/"${ROOT_PART_NAME}" | cut -d "/" -f 4) #mmcblk0
  ROOT_DEV="/dev/${ROOT_DEV_NAME}" #/dev/mmcblk0
  ROOT_PART_NUM=$(cat "/sys/block/${ROOT_DEV_NAME}/${ROOT_PART_NAME}/partition") #2

  BOOT_PART_DEV=$(findmnt /boot -o source -n) #/dev/mmcblk0p1
  BOOT_PART_NAME=$(echo "$BOOT_PART_DEV" | cut -d "/" -f 3) #mmcblk0p1
  BOOT_DEV_NAME=$(echo /sys/block/*/"${BOOT_PART_NAME}" | cut -d "/" -f 4) #mmcblk0
  BOOT_PART_NUM=$(cat "/sys/block/${BOOT_DEV_NAME}/${BOOT_PART_NAME}/partition") #1
  
  OLD_DISKID=$(fdisk -l "$ROOT_DEV" | sed -n 's/Disk identifier: 0x\([^ ]*\)/\1/p') #9730496b

  #ROOT_DEV_SIZE=$(cat "/sys/block/${ROOT_DEV_NAME}/size") #Size of SD-card in 512 bytes: 30930944 (number of 512 byte sectors)
  #TARGET_END=$((ROOT_DEV_SIZE - 1)) # Disk end sector 30930943
 
  TARGET_END="3907583" # Sector end for Root partion, for performace make sure Sectores match a multi of 2048...  3907583 = ~1.6GB
  
  SD_PART_NUM=$((ROOT_PART_NUM + 1))
  SD_PART_DEV="$ROOT_DEV"p"$SD_PART_NUM"
  SD_DEV_SIZE=$(cat "/sys/block/${ROOT_DEV_NAME}/size") # Size of root partition: 30930944 (number of 512 byte sectors)
  SD_START=$((TARGET_END + 1))
  SD_END=$((SD_DEV_SIZE - 1))
  echo "SD partition number: "$SD_PART_NUM
  echo "SD Partition:        "$SD_PART_DEV
  echo "SD Size:             "$SD_DEV_SIZE
  echo "SD Start:            "$SD_START
  echo "SD End:              "$SD_END
   
  PARTITION_TABLE=$(parted -m "$ROOT_DEV" unit s print | tr -d 's')
  # BYT;
  # /dev/mmcblk0:30930944:d/mmc:512:512:mdo:SD USDU1:;
  # 1:8192:532479:524288:fat32::lba;
  # 2:532480:3661823:3129344:ext4::;

  LAST_PART_NUM=$(echo "$PARTITION_TABLE" | tail -n 1 | cut -d ":" -f 1) #2
  
  ROOT_PART_LINE=$(echo "$PARTITION_TABLE" | grep -e "^${ROOT_PART_NUM}:") #2:532480:3661823:3129344:ext4::;
  ROOT_PART_START=$(echo "$ROOT_PART_LINE" | cut -d ":" -f 2) #532480
  ROOT_PART_END=$(echo "$ROOT_PART_LINE" | cut -d ":" -f 3) #3661823
}

mount -t proc proc /proc
#mount -t sysfs sys /sys
mount -t tmpfs tmp /run
mkdir -p /run/systemd

#mount /boot
mount / -o remount,ro

#sed -i 's| init=/usr/lib/raspi-config/init_resize\.sh||' /boot/cmdline.txt
#sed -i 's| sdhci\.debug_quirks2=4||' /boot/cmdline.txt

#if ! grep -q splash /boot/cmdline.txt; then
#  sed -i "s/ quiet//g" /boot/cmdline.txt
#fi

mount /boot -o remount,ro
sync
echo 1 > /proc/sys/kernel/sysrq

get_variables

echo "parted -m $ROOT_DEV u s resizepart $ROOT_PART_NUM $TARGET_END"
parted -m "$ROOT_DEV" u s resizepart "$ROOT_PART_NUM" "$TARGET_END"

echo "parted -m $ROOT_DEV u s mkpart primary fat32 $SD_START $SD_END"
parted -m -s "$ROOT_DEV" u s mkpart primary fat32 "$SD_START" "$SD_END"

fix_partuuid

sudo resize2fs $ROOT_PART_DEV
sudo mkfs.msdos $SD_PART_DEV

reboot -f