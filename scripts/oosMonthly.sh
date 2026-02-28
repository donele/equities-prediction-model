lastMonth=`get_idate -b -dd 30 -m US`
yyyymm=${lastMonth:0:6}
d1=${yyyymm}01
d2=${yyyymm}31

for m in US UF CA EU AT AH AS AA KR; do
	pmodel.sh $m sample
	wait
	(pmodel.sh $m pred 0 $d1 $d2 && \
	pmodel.sh $m ana 0 $d1 $d2)&
done


