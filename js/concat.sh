#!/bin/bash

cat header $(find ../trace -name "rank_*.json" | sort -n -k 5 -t _ | sort -s -n -k 4 -t _ | sort -s -n -k 3 -t _ | sort -s -n -k 2 -t _) footer > rankall.json

