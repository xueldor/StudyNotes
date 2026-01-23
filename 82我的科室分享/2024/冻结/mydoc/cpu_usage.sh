# 适用android

ldifs="$IFS"
IFS=$'\n'
get_cpu_usage() {
    prev_total=0
    prev_idle=0

    while true; do
        cpu_stats=$(cat /proc/stat | grep "^cpu")
        total=0
        idle=0

        index=-1;
        for cpu in $cpu_stats; do
            IFS=' '
            fields=(`echo $cpu | tr ' '`)
            IFS=$'\n'

            if [ "${fields[0]}" != "cpu" ]; then
                index=$((index+1))

                user=${fields[1]}
                nice=${fields[2]}
                system=${fields[3]}
                idle=${fields[4]}
                iowait=${fields[5]}
                irq=${fields[6]}
                softirq=${fields[7]}

                total=$((user + nice + system + idle + iowait + irq + softirq))

                diff_total=$((total - prev_total[index]))
                diff_idle=$((idle - prev_idle[index]))

                usage=$(( (1000 * (diff_total - diff_idle) / diff_total + 5) / 10 ))

                echo "${fields[0]}: $usage%"

                prev_total[index]=$total
                prev_idle[index]=$idle
            fi
        done
        sleep 1
        echo "---"
    done
}

get_cpu_usage

IFS="$oldifs"
