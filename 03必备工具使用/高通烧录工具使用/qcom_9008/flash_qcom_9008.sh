#!/bin/sh
start_seconds=$(date +%s)
# lsusb查到：05c6:9008，/etc/udev/rules.d/70-ttyusb.rules添加一行：
# KERNEL=="ttyUSB[0-9]*",ATTRS{idVendor}=="05c6", ATTRS{idProduct}=="9008",MODE="0666",SYMLINK+="tty_qcom_9008"
# 避免每次都要修改tty_path的值
tty_path=/dev/tty_qcom_9008
# 按实际修改下路径
UFS_PATH=/media/xue/workspace/626_8155/ROM/flat_ufs
#UFS_PATH=$1

./QSaharaServer -v 2 -p $tty_path -s 13:$UFS_PATH/prog_firehose_ddr.elf
#./fh_loader --port=$tty_path --sendxml=rawprogram_unsparse0.xml --search_path=$UFS_PATH --noprompt --showpercentagecomplete --memoryname=UFS
#./fh_loader --port=$tty_path --sendxml=patch0.xml --search_path=$UFS_PATH --noprompt --showpercentagecomplete --memoryname=UFS
#./fh_loader --port=$tty_path --sendxml=rawprogram1.xml --search_path=$UFS_PATH --noprompt --showpercentagecomplete --memoryname=UFS
#./fh_loader --port=$tty_path --sendxml=patch1.xml --search_path=$UFS_PATH --noprompt --showpercentagecomplete --memoryname=UFS
#./fh_loader --port=$tty_path --sendxml=rawprogram2.xml --search_path=$UFS_PATH --noprompt --showpercentagecomplete --memoryname=UFS
#./fh_loader --port=$tty_path --sendxml=patch2.xml --search_path=$UFS_PATH --noprompt --showpercentagecomplete --memoryname=UFS
#./fh_loader --port=$tty_path --sendxml=rawprogram3.xml --search_path=$UFS_PATH --noprompt --showpercentagecomplete --memoryname=UFS
#./fh_loader --port=$tty_path --sendxml=patch3.xml --search_path=$UFS_PATH --noprompt --showpercentagecomplete --memoryname=UFS
#./fh_loader --port=$tty_path --sendxml=rawprogram4.xml --search_path=$UFS_PATH --noprompt --showpercentagecomplete --memoryname=UFS
#./fh_loader --port=$tty_path --sendxml=patch4.xml --search_path=$UFS_PATH --noprompt --showpercentagecomplete --memoryname=UFS
#./fh_loader --port=$tty_path --sendxml=rawprogram5.xml --search_path=$UFS_PATH/ --noprompt --showpercentagecomplete --memoryname=UFS
#./fh_loader --port=$tty_path --sendxml=patch5.xml --search_path=$UFS_PATH/ --noprompt --showpercentagecomplete --memoryname=UFS

# same as
XMLFILES=rawprogram_unsparse0.xml,patch0.xml,rawprogram1.xml,patch1.xml,rawprogram2.xml,patch2.xml,rawprogram3.xml,patch3.xml,rawprogram4.xml,patch4.xml,rawprogram5.xml,patch5.xml
./fh_loader --port=$tty_path --sendxml=$XMLFILES --search_path=$UFS_PATH --noprompt --showpercentagecomplete --memoryname=UFS

end_seconds=$(date +%s)
elapsed_time=$((end_seconds - start_seconds))
minutes=$(( (elapsed_time / 60) ))
seconds=$(( elapsed_time % 60 ))
printf "cost time: %02d分%02d秒\n" "$minutes" "$seconds"
