dateTo=`get_idate -m US -dd 3 -lt -b`
udate=${dateTo:0:6}01

recentFitFile=`ls $HOME/hffit/US/fit/tar60\;0/coef/coef*.txt | grep -v $udate | sort | tail -n 1`
basename=${recentFitFile##*/}
compdate=${basename:4:8}

logfile=$HOME/hffit2/log/fitNonlinearModels.log
echo udate $udate dateTo $dateTo recentFitFile $basename compdate $compdate > $logfile 2>&1

for m in AS AA KR; do
	pmodel.sh $m sample
	pmodel.sh $m fit $udate
	wait
	(pmodel.sh $m pred $compdate $udate $dateTo && \
	pmodel.sh $m pred $udate $udate $dateTo && \
	pmodel.sh $m ana $compdate $udate $udate $dateTo)&
done


