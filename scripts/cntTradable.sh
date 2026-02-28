#!/bin/bash
predFile=$1
awk 'NR>1 {
	{cnt += 1} \
	{cost = $4/2 + 36/$5} \
	{sameSign = $1 * $3 > 0} \
	{profitable = ($1 > cost || -$1 > cost)} \
	{tradable = ($3 > cost || -$3 > cost)} \

	{if(profitable) {if($1 > 0) {edge = $1 - cost} else {edge = -$1 - cost}} else {edge = 0} } \
	{if(tradable) {if($3 > 0) {tedge = $1 - cost} else {tedge = -$1 - cost}} else {tedge = 0} } \
	{if(tradable) {if($3 > 0) {pedge = $3 - cost} else {pedge = -$3 - cost}} else {pedge = 0} } \
	{totEdge += edge} \
	{totTradableEdge += tedge} \
	{totPredEdge += pedge} \
	{if(sameSign) {totTradableEdgeSS += tedge} else {totTradableEdgeOS += tedge}} \
	{if(sameSign) {totPredEdgeSS += pedge} else {totPredEdgeOS += pedge}} \

	{if(sameSign && tradable) cntTradableSS += 1} \
	{if(!sameSign && tradable) cntTradableOS += 1} \
	{if(tradable) tradableEdge += edge} \
	{if(profitable) cntProfitable += 1} \
	{if(tradable) cntTradable += 1} \
	{if(sameSign && profitable && tradable) cntTradeProfit += 1}
}; END {
	printf("%d %.2f%% %.2f%% %.2f%% %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n",
	cnt, cntProfitable/cnt*100, cntTradable/cnt*100, cntTradeProfit/cnt*100,
	totEdge/cntProfitable, totTradableEdge/cntTradable, totPredEdge/cntTradable,
	totTradableEdgeSS/cntTradableSS, totTradableEdgeOS/cntTradableOS,
	totPredEdgeSS/cntTradableSS, totPredEdgeOS/cntTradableOS)
}' $1
