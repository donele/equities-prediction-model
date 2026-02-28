#!/bin/bash
if (( $# < 3 )); then
	echo "  ${0##*/} <model> <sigType> <udate> <idate> <nmax>"
	exit 1
fi
model=$1
sigType=$2
udate=$3
idate=$4
nmax=$5

#varOm="time sprd logVolu logPrice qI mret60 TOBoff1 mret300 mret5 exchange medVolat hilo mret30 BsRat1 BsRat2 BqI2 Boff1 Boff2 BOffmedVol.01 mret15 BmedSprdqI1 BmedSprdqI2 BmedSprdqI4 mret120 mret600 hiloLag1Rat hiloLag1 bollinger300 bollinger900"
#varTm="time logVolu logPrice logMedSprd mret300 mret300L mret600L mret1200L m2400L m4800L mretONLag0 mretIntraLag1 vm600 vm3600 qIWt qIMax hilo TOBqI2 TOBqI3 TOBoff1 TOBoff2 sprd exchange northRST northTRD northBP hiloQAI medVolat prevDayVolu mretOpen qIMax2 mretONLag1 mretIntraLag2 vmIntra isSecTypeF sprdOpen BOffmedVol.01 BOffmedVol.02 BOffmedVol.04 BOffmedVol.08 BOffmedVol.16 BOffmedVol.32 BqRat1 BqRat2 BsRat1 BsRat2 BqI2 BqI3 Boff1 Boff2 BmedSprdqI.25 BmedSprdqI.5 BmedSprdqI1 BmedSprdqI2 BmedSprdqI4 BmedSprdqI8 BmedSprdqI16 mI600 mI3600 mIIntra hiloLag1Open hiloLag2Open hiloLag1Rat hiloLag2Rat hiloQAI2 hiloLag1 hiloLag2 hilo900 bollinger300 bollinger900"

varOm="time sprd logVolu logPrice qI mret60 TOBoff1 mret300 mret5 relVolat medVolat hilo mret30 BsRat1 BsRat2 BqI2 Boff1 Boff2 BOffmedVol.01 mret15 BmedSprdqI1 BmedSprdqI2 BmedSprdqI4 mret120 mret600 hiloLag1Rat hiloLag1 bollinger300 relSprd fillImb vm15 vm30 vm60 vm120 vmIntra volSclBsz volSclAsz cwret30 cwret60 cwret120 rtrd mrtrd dnmk snmk"

varTm="time logVolu logPrice logMedSprd mret300 mret300L mret600L mret1200L m2400L m4800L mretONLag0 mretIntraLag1 vm600 vm3600 qIWt qIMax hilo TOBqI2 TOBqI3 TOBoff1 TOBoff2 sprd exchange northRST northTRD northBP hiloQAI medVolat prevDayVolu mretOpen qIMax2 mretONLag1 mretIntraLag2 vmIntra vm300 sprdOpen BOffmedVol.01 BOffmedVol.02 BOffmedVol.04 BOffmedVol.08 BOffmedVol.16 BOffmedVol.32 BqRat1 BqRat2 BsRat1 BsRat2 BqI2 relVolat Boff1 Boff2 relSprd BmedSprdqI.5 BmedSprdqI1 BmedSprdqI2 BmedSprdqI4 BmedSprdqI8 BmedSprdqI16 mI600 mI3600 mIIntra hiloLag1Open hiloLag2Open hiloLag1Rat hiloLag2Rat hiloQAI2 hiloLag1 hiloLag2 hilo900 bollinger300 bollinger900 volSclBsz volSclAsz cwret300 cwret300L rtrd mrtrd dnmk snmk"

varList=$varTm
sampleFreq=1
targetName=tar600\;60_1.0_tar3600\;660_0.5
if [[ "x${sigType}" == "xom" ]]; then
	varList=$varOm
	sampleFreq=10
	targetName=tar60\;0
fi

if [[ "x$nmax" == "x" ]]; then
	sigDump -n 2000000 -d $idate -v $varList | awk '((NR-1)%'$sampleFreq'==0){print $0}' > temp_sig.txt
	head -n 2000001 ../../../$model/fit/$targetName/stat_${udate}/preds/pred${idate}.txt | awk '((NR-1)%'$sampleFreq'==0){print $3}' > temp_pred.txt
else
	sigDump -n 2000000 -d $idate -v $varList | awk '((NR-1)%'$sampleFreq'==0){print $0}' | head -n $nmax > temp_sig.txt
	head -n 2000001 ../../../$model/fit/$targetName/stat_${udate}/preds/pred${idate}.txt | awk '((NR-1)%'$sampleFreq'==0){print $3}' | head -n $nmax > temp_pred.txt
fi

nlinesSig=`wc temp_sig.txt | awk '{print $1}'`
nlinesPred=`wc temp_pred.txt | awk '{print $1}'`
if (( $nlinesSig != $nlinesPred )); then
	echo $nlinesSig != $nlinesPred
	exit 1
fi

awk '((NR-1)%'$sampleFreq'==0){print $1"\t"$2}' sig${idate}.txt | head -n $nlinesSig > temp_head.txt

paste temp_head.txt temp_pred.txt temp_sig.txt > pred_input_${model}_${sigType}_${udate}_${idate}.txt
rm temp_head.txt temp_sig.txt temp_pred.txt

