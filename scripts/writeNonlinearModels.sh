udate=$1

for m in US UF CA EU AT; do
	checkParams -om -tm -ome -m $m
	pmodel.sh $m write $udate
	checkParams -om -tm -ome -m $m
done

for m in AH AS AA KR; do
	checkParams -om -tm -m $m
	pmodel.sh $m write $udate
	checkParams -om -tm -m $m
done


