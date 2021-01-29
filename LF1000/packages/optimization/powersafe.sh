#!/bin/sh

WHERE=/LF/Bulk/test
VERBOSE=false

inventory () # where to
{
	find $1 -type f | xargs md5sum > $2
}

verify () # to
{
	md5sum -c $1 | fgrep -v ': OK'
}

dirs ()
{
	find $1 -type d
}

files ()
{
	find $1 -type f
}

rand_line () # file -- line from file
{
	lines=$(cat $1 | wc -l)
	if [ $lines = 0 ]; then return; fi
	line=$((1 + RANDOM % lines))
	head -$line $1 | tail -1
}

rand_dir ()
{
	t=$(mktemp -t)
	dirs $WHERE > $t;
	rand_line $t
	rm $t
}

rand_file ()
{
	t=$(mktemp -t)
	files $WHERE > $t;
	rand_line $t
	rm $t
}

new_dir ()
{
	d=$(rand_dir)
	if [ -z "$d" ]; then d=$WHERE; fi
	d1=$(mktemp -d $d/dir.XXXXXX)
	$VERBOSE && echo "adding directory $d1"
}

rm_dir ()
{
	d=$(rand_dir)
	if [ -z "$d" ]; then return; fi
	if [ "$d" = "$WHERE" ]; then return; fi
	$VERBOSE && echo "removing directory $d"
	rm -rf $d
}

new_file ()
{
	d=$(rand_dir)
	f=$(mktemp $d/file.XXXXXX)
	echo "$d/$f" > $f
	$VERBOSE && echo "adding $f"
}

rm_file ()
{
	f=$(rand_file)
	$VERBOSE && echo "removing file $f"
	rm -f $f
}

# inventory $WHERE /inventory.md5
# echo "foo" >> $WHERE/Data/bar
# verify /inventory.md5

rm -rf $WHERE
mkdir -p $WHERE
i=0
while true; do
	for j in $(seq 1 3); do
		new_dir
		new_file
		new_file
		new_file
	done
	rm_file
	rm_dir
	rm_file
	# echo "-------------"
	# find $WHERE | sort
	echo "Pass $i " \
	     $(find $WHERE/ -type f | wc -l) "files and" \
	     $(find $WHERE/ -type d | wc -l) "dirs"
	echo "-------------"
	i=$((++i))
done


# Create a file
# Modify a file
# remove a file
# Create a directory
# Remove a directory



