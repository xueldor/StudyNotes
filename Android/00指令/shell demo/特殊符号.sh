echo "pid is $$"
echo "bash name is $0"
echo "arg count is $#"
echo "print arg: $*"
echo "print arg: $@"
for var in $*
do 
	echo $var
done
for var in "$*"
do 
	echo $var
done
for var in $@
do 
	echo $var
done
for var in "$@"
do 
	echo $var
done
