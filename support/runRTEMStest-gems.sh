#!/bin/bash

if [[ $# -ne 6 ]]
then
echo "Error - parameters missing"
echo "Syntax : $0 output_dir path_to_iso test_group test_name path_to_source path_to_symbols"
echo "example : $0 output /svn/rtemssparc64/rtems/boot/image.iso samples hello /svn/rtemssparc64/rtems/rtemscvs /svn/rtemssparc64/rtems/boot/debug/rtems/"
exit 1
fi

OUTDIR=$1
ISOPATH=$2
ISOSIZE=`ls -la $ISOPATH  | awk '{print $5}'`
TESTGROUP=$3
TESTNAME=$4
SOURCEPATH=$5
SYMBOLS=$6

ISOPATH=`echo $ISOPATH | sed 's/\//\\//g'`
OUTDIR=`echo $OUTDIR | sed 's/\//\\//g'`
SOURCEPATH=`echo $SOURCEPATH | sed 's/\//\\//g'`
SYMBOLS=`echo $SYMBOLS | sed 's/\//\\//g'`

rm ${OUTDIR}/$TESTNAME.txt

#cat targets/niagara-simple/niagara-simple-RTEMS-rubyTESTtemplate.simics | sed s/REPLACEDBYSED534ISOSIZE/$ISOSIZE/ | sed s/REPLACEDBYSED534TESTNAME/$TESTNAME/ | sed "s;REPLACEDBYSED534ISOPATH;$ISOPATH;"
#cat targets/niagara-simple/niagara-simple-RTEMS-rubyTESTtemplate.simics | sed s/REPLACEDBYSED534ISOSIZE/$ISOSIZE/ | sed s/REPLACEDBYSED534TESTGROUP/$TESTGROUP/ | sed s/REPLACEDBYSED534TESTNAME/$TESTNAME/ | sed "s;REPLACEDBYSED534ISOPATH;$ISOPATH;" | sed "s;REPLACEDBYSED534SOURCEPATH;$SOURCEPATH;" | sed "s;REPLACEDBYSED534SYMBOLS;$SYMBOLS;" > temp.simics
cat targets/serengeti/sun4u.rtems4.11-template-gems.simics | sed s/REPLACEDBYSED534ISOSIZE/$ISOSIZE/ | sed s/REPLACEDBYSED534TESTGROUP/$TESTGROUP/ | sed s/REPLACEDBYSED534TESTNAME/$TESTNAME/ | sed "s;REPLACEDBYSED534ISOPATH;$ISOPATH;" | sed "s;REPLACEDBYSED534SOURCEPATH;$SOURCEPATH;" | sed "s;REPLACEDBYSED534SYMBOLS;$SYMBOLS;" > temp.simics
./simics -no-win -stall temp.simics 
#./simics -stall temp.simics 
#&
PID1=$!

#./check-live.sh $PID1 $TESTNAME &
PID2=$!

#wait ${PID1} ${PID2}
if [[ $? -ne 0 ]]
then
	echo "***** Exiting with -1 *****"
	exit -1
fi

cat $TESTNAME.txt | sed '/^\s*$/d' | sed '/END_TOKEN/d' | sed 's/\x0D$//' > temp.txt
rm $TESTNAME.txt
mv temp.txt ${OUTDIR}/$TESTNAME.txt
diff ${OUTDIR}/$TESTNAME.txt results-expected/$TESTNAME.scn -bU 0 --strip-trailing-cr > ${OUTDIR}/$TESTNAME.diff
mv $TESTNAME.ruby ${OUTDIR}/$TESTNAME.ruby
mv $TESTNAME.opal ${OUTDIR}/$TESTNAME.opal
echo "Results in ${OUTDIR}/$TESTNAME.*"
exit 0
